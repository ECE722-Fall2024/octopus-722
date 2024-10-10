/*
 * File  :      CacheController.cpp
 * Author:      Mohammed Ismail
 * Email :      ismaim22@mcmaster.ca
 *
 * Created On June 23, 2021
 */

#include "../header/CacheController.h"
#define DEBUG_MSGS
namespace ns3
{
    // override ns3 type
    TypeId CacheController::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::CacheController").SetParent<Object>();
        return tid;
    }

    // private controller constructor
    CacheController::CacheController(CacheXml &cacheXml, string &fsm_path, CommunicationInterface *upper_interface,
                                     CommunicationInterface *lower_interface, bool cach2Cache,
                                     vector<int> sharedMemId, CohProtType pType, vector<int>* private_caches_id)
    {
        m_cache_cycle = 1;
        m_nbanks = cacheXml.GetNBanks();
        m_bankNumber = cacheXml.GetBankNum();
        m_core_id = cacheXml.GetCacheId();
        m_shared_memory_id = sharedMemId;

        m_dt = cacheXml.GetCtrlClkNanoSec();
        m_clk_skew = m_dt * cacheXml.GetCtrlClkSkew() / 100.00;

        m_upper_interface = upper_interface;
        m_lower_interface = lower_interface;

        ReplacementPolicy* policy = Policy::getReplacementPolicy(cacheXml.GetReplcPolicy(), cacheXml.GetNWays());
        // m_cache = new CacheDataHandler(cacheXml);
        m_data_handler = new CacheDataHandler_COTS(cacheXml, policy);

        m_cache_line_size = cacheXml.GetBlockSize();

        m_protocol = Protocols::getNewProtocol(pType, m_data_handler, fsm_path, m_core_id, m_shared_memory_id[0]);

        m_processing_queue =
            new FRFCFS_Buffer<Message, CoherenceProtocolHandler>(&CoherenceProtocolHandler::getRequestState,
                                                                 m_protocol,
                                                                 cacheXml.GetNPendReq());
                                                                         
        m_data_access_arbiter = (private_caches_id == NULL) ? NULL : new RRFCFSArbiter(private_caches_id, cacheXml.GetDataAccessLatency());
    }



    CacheController::~CacheController()
    {
        delete m_protocol;
        delete m_data_handler;
    }

    void CacheController::cycleProcess()
    {
        m_data_handler->updateCycle(m_cache_cycle);
        m_protocol->updateCycle(m_cache_cycle);
        this->processDataArrayBuffer();
        this->processLogic(); // Call cache controller

        Simulator::Schedule(NanoSeconds(m_dt), &CacheController::step, Ptr<CacheController>(this)); // Schedule the next run
        m_cache_cycle++;
    }

    void CacheController::init()
    {
        m_protocol->initializeCacheStates(); // Initialized Cache Coherence Protocol
        Simulator::Schedule(NanoSeconds(m_clk_skew), &CacheController::step, Ptr<CacheController>(this));
    }

    void CacheController::step(Ptr<CacheController> cache_controller)
    {
        cache_controller->cycleProcess();
    }

    void CacheController::callActionFunction(ControllerAction action)
    {
        switch (action.type)
        {
            case ControllerAction::Type::REMOVE_PENDING: this->removePendingAndRespond(action.data); return;
            case ControllerAction::Type::HIT_Action: this->hitAction(action.data); return;
            case ControllerAction::Type::ADD_PENDING: this->addtoPendingRequests(action.data); return;
            case ControllerAction::Type::SEND_BUS_MSG: this->sendBusRequest(action.data); return;
            case ControllerAction::Type::WRITE_BACK: this->performWriteBack(action.data); return;
            case ControllerAction::Type::UPDATE_CACHE_LINE: this->updateCacheLine(action.data); return;
            case ControllerAction::Type::WRITE_CACHE_LINE_DATA: this->writeCacheLineData(action.data); return;
            case ControllerAction::Type::SAVE_REQ_FOR_WRITE_BACK: this->saveReqForWriteBack(action.data); return;
            case ControllerAction::Type::NO_ACTION: this->noAction(action.data); return;
            case ControllerAction::Type::STALL: this->stall(action.data); return;

            default: cout << "CacheController: Invalid Action Type!!" << endl; return;
        }
    }

    void CacheController::processLogic()
    {
        this->addRequests2ProcessingQueue(*m_processing_queue);

        while(true)
        {
            Message ready_msg;
            if (m_processing_queue->getFirstReady(&ready_msg) == false)
                return;

            if(ready_msg.source == Message::Source::LOWER_INTERCONNECT)
                Logger::getLogger()->updateRequest(ready_msg.msg_id, Logger::EntryId::CACHE_CHECKPOINT);

            // parser: print timestamp that msg is being processed
            if (ready_msg.source == Message::LOWER_INTERCONNECT)
            {
                std::cerr << ready_msg.msg_id << "," << ready_msg.addr << "," \
                    << "msgProc" << "," <<  m_core_id << ","<< m_cache_cycle<<"\n";
            }

            vector<ControllerAction> actions = m_protocol->processRequest(ready_msg);
            
            
            for (ControllerAction action : actions)
                callActionFunction(action);
        }
    }

    void CacheController::processDataArrayBuffer()
    {
        if(m_data_handler->isReady() && m_data_access_arbiter != NULL)
        {
            Message selected_msg;
            vector<vector<Message>*> messages_pending_data_access; //wrapper vector to use the arbiter
            messages_pending_data_access.push_back(&m_data_access_buffer);

            bool msg_available = m_data_access_arbiter->elect(m_cache_cycle, 
                                                              messages_pending_data_access, &selected_msg);
            if(msg_available)
            {
                Logger::getLogger()->updateRequest(selected_msg.msg_id, Logger::EntryId::CACHE_CHECKPOINT);
                std::cerr << selected_msg.msg_id << "," << selected_msg.addr << "," \
                    << "dataRdy" << "," <<  m_core_id << ","<< m_cache_cycle<<"\n";
                callActionFunction(m_data_access_action[selected_msg.msg_id]);
                m_data_access_action.erase(selected_msg.msg_id);
            }
        }
    }

    void CacheController::addRequests2ProcessingQueue(FRFCFS_Buffer<Message, CoherenceProtocolHandler> &buf)
    {
        Message msg;
        // Look at the interfaces for new messages; if so, serialize them
        // by adding them to the message queue
        if (m_upper_interface->peekMessage(&msg))
        {
            if ((uint64_t)this->m_bankNumber  == calculate_bank(msg.addr))
            {
               msg.source = Message::Source::UPPER_INTERCONNECT;
               if (buf.pushFront(msg)) {
                   m_upper_interface->popFrontMessage();
                   std::cerr << msg.msg_id << "," << msg.addr << "," \
                       << "add2q_u" << "," <<  m_core_id << ","<< m_cache_cycle<<"\n";
                }
            }
            else
            {
                m_upper_interface->popFrontMessage();

            }
        }

        if (m_lower_interface->peekMessage(&msg))
        {          

            if ((uint64_t)this->m_bankNumber  == calculate_bank(msg.addr))
            {
                msg.source = Message::Source::LOWER_INTERCONNECT;
                if (buf.pushBack(msg, FRFCFS_State::NonReady)) {
                   m_lower_interface->popFrontMessage();
                   std::cerr << msg.msg_id << "," << msg.addr << "," \
                    << "add2q_l" << "," <<  m_core_id << ","<< m_cache_cycle<<"\n";
                }
            }
            else
            {
                m_lower_interface->popFrontMessage();
            }

        }

        this->checkReplacements(buf);
    }

    uint64_t CacheController::getAddressKey(uint64_t addr)
    {
        
        return (addr >> ((int)log2(this->m_cache_line_size)+(int)log2(this->m_nbanks)));
    }

    void CacheController::addtoPendingRequests(void *data_ptr)
    {
        Message *msg = (Message *)data_ptr;
        this->m_pending_cpu_requests[this->getAddressKey(msg->addr)].push(*msg);
        std::cerr << msg->msg_id << "," << msg->addr << "," \
            << "add_req" << "," <<  m_core_id << ","<< m_cache_cycle<<"\n";
        delete msg;
    }

    void CacheController::removePendingAndRespond(void *data_ptr)
    {
        Message *msg = (Message *)data_ptr;
        // check if the message is in the pending cpu requests map
        if (m_pending_cpu_requests.find(getAddressKey(msg->addr)) != m_pending_cpu_requests.end())
        {
            queue<Message> pending_messages = this->m_pending_cpu_requests[this->getAddressKey(msg->addr)];
            if (pending_messages.size() > 1)
                cout << "How !!!!!1" << endl;
            while (!pending_messages.empty())
            {
                if (msg->data != NULL)
                {
                    // Update values of message in pending map
                    // with updated values from msg
                    pending_messages.front().complementary_value = msg->complementary_value;
                    pending_messages.front().to = msg->to;
                    pending_messages.front().copy(msg->data);
                }
                else // This can happen while moving from O to M
                    cout << "CacheController: Remove from pending without data" << endl;
           
//                if (m_core_id != 10) {
//                    std::cerr << msg->msg_id << "," << msg->addr << ","
//                    << "termina" << "," <<  m_core_id << ","<< m_cache_cycle << "\n";
//                }
                std::cerr << msg->msg_id << "," << msg->addr << "," \
                    << "respond" << "," <<  m_core_id << ","<< m_cache_cycle << "\n";
                // push updated message to lower interface as DATA_RESPONSE
                if (!m_lower_interface->pushMessage(pending_messages.front(), this->m_cache_cycle, MessageType::DATA_RESPONSE))
                {
                    cout << "CacheController: Cannot insert the Msg into lower interface." << endl;
                    exit(0);
                }
                pending_messages.pop();
            }
            this->m_pending_cpu_requests.erase(this->getAddressKey(msg->addr));
        }
        else
        { // For the LLC
            if (msg->data == NULL)
            {
                bool is_in_buffer = false;
                // first the the LLC checks if a more recent copy of
                // the requested data is pending in the data access buffer
                for (Message i : m_data_access_buffer)
                {
                    if (i.addr == msg->addr && 
                        i.data != NULL)
                    {
                        is_in_buffer = true;
                        msg->copy(i.data);
                        break;
                    }
                }
                // if the address is not in the buffer, the LLC attempts
                // to respond with data from its cache data array
                if (!is_in_buffer) 
                {
                    if(!checkReadinessOfCache(*msg, 
                        ControllerAction::Type::REMOVE_PENDING, data_ptr))
                    {
                        std::cerr << msg->msg_id << "," << msg->addr << "," \
                            << "datNrdy" << "," 
                            <<  m_core_id << ","<< m_cache_cycle << "\n";
                        return;
                    }
                    GenericCacheLine cache_line;
                    m_data_handler->readCacheLine(msg->addr, &cache_line);
                    msg->copy(cache_line.m_data);
                }
            }

            std::cerr << msg->msg_id << "," << msg->addr << "," \
                    << "respond" << "," <<  m_core_id << ","<< m_cache_cycle << "\n";
            // push message to lower interface as DATA_RESPONSE
            if (!m_lower_interface->pushMessage(*msg, this->m_cache_cycle, 
                MessageType::DATA_RESPONSE))
            {
                cout << "CacheController: Cannot insert the Msg into lower interface." << endl;
                exit(0);
            }
        }

        delete msg;
    }

    void CacheController::hitAction(void *data_ptr)
    {
        Message *msg = (Message *)data_ptr;

        if (msg->data == NULL)
        {
            if(!checkReadinessOfCache(*msg, ControllerAction::Type::HIT_Action, 
                data_ptr)) {
                std::cerr << msg->msg_id << "," << msg->addr << "," \
                    << "datNrdy" << "," <<  m_core_id << ","<< m_cache_cycle<<"\n";
                return;
            }
            GenericCacheLine cache_line;
            m_data_handler->readCacheLine(msg->addr, &cache_line);
            msg->copy(cache_line.m_data);
        }

        std::cerr << msg->msg_id << "," << msg->addr << "," 
                << "hitActn" << "," <<  m_core_id << ","<< m_cache_cycle<<"\n";
        //if (m_core_id != 10) {
        //    std::cerr << msg->msg_id << "," << msg->addr << "," 
        //            << "termina" << "," <<  m_core_id << ","<< m_cache_cycle<<"\n";
        //}

        if (!m_lower_interface->pushMessage(*msg, this->m_cache_cycle, MessageType::DATA_RESPONSE))
        {
            cout << "CacheController: Cannot insert the Msg into lower interface." << endl;
            exit(0);
        }

        delete msg;
    }

    void CacheController::sendBusRequest(void *data_ptr)
    {
        Message *msg = (Message *)data_ptr;
        msg->cycle = this->m_cache_cycle;

        if (!m_upper_interface->pushMessage(*msg, this->m_cache_cycle, MessageType::REQUEST))
        {
            cout << "CacheController(id = " << this->m_core_id << "): Cannot insert the Msg into the upper interface FIFO, FIFO is Full" << endl;
            exit(0);
        }

        delete msg;
    }

    void CacheController::performWriteBack(void *data_ptr)
    {
        Message *msg = (Message *)data_ptr;
        uint64_t original_msg_id = msg->msg_id;
        if (this->m_saved_requests_for_wb.find(this->getAddressKey(msg->addr)) !=
            this->m_saved_requests_for_wb.end())
        {
            msg->owner = this->m_saved_requests_for_wb[this->getAddressKey(msg->addr)].owner;
            msg->msg_id = this->m_saved_requests_for_wb[this->getAddressKey(msg->addr)].msg_id;
            this->m_saved_requests_for_wb.erase(this->getAddressKey(msg->addr));
        }

        if(msg->data == NULL)
        {
            if(!checkReadinessOfCache(*msg, ControllerAction::Type::WRITE_BACK, 
                data_ptr)) {
                std::cerr << msg->msg_id << "," << msg->addr << "," \
                    << "datNrdy" << "," <<  m_core_id << ","<< m_cache_cycle<<"\n";
                return;
            }
            GenericCacheLine cache_line;
            m_data_handler->readCacheLine(msg->addr, &cache_line);
            msg->copy(cache_line.m_data);
        }

        if (msg->owner == this->m_core_id || (m_children_controllers.size() == 1 && m_children_controllers[0]->getId() == msg->owner))
            msg->to.push_back(this->m_shared_memory_id);
        else
            msg->to.push_back(msg->owner);

        std::cerr << msg->msg_id << "," << msg->addr << "," \
            << "writeBk" << "," <<  m_core_id << ","<< m_cache_cycle<<"\n";
        if (m_core_id != 10)
        {
            std::cerr << msg->msg_id << "," << msg->addr << "," \
                << "L1WBsrc" << "," <<  m_core_id << "," \
                << original_msg_id <<"\n";
        }
        // If the LLC is writing back, then the message is complete
        // (evicted message from LLC writes back)
        //if (m_core_id == 10) {
        //    std::cerr << msg->msg_id << "," << msg->addr << "," 
        //        << "termina" << "," <<  m_core_id << ","<< m_cache_cycle<<"\n";
        //}

        if (!m_upper_interface->pushMessage(*msg, this->m_cache_cycle, MessageType::DATA_RESPONSE))
        {
            cout << "CacheController: Cannot insert the Msg into BusTxResp FIFO, FIFO is Full" << endl;
            exit(0);
        }

        delete msg;
    }

    void CacheController::updateCacheLine(void *data_ptr)
    {
        Message *msg = (Message *)data_ptr;
        GenericCacheLine *cache_line = (GenericCacheLine *)((uint8_t *)data_ptr + sizeof(Message));
        m_data_handler->updateLineBits(msg->addr, cache_line);
        //if (msg->complementary_value == 2) {
        //            std::cerr << msg->msg_id << "," << msg->addr << "," 
        //    << "replace" << "," <<  m_core_id << ","<< m_cache_cycle<<"\n";
        //    if (msg->owner != 10) {
        //             std::cerr << msg->msg_id << "," << msg->addr << "," 
        //            << "termina" << "," <<  m_core_id << ","<< m_cache_cycle<<"\n";
        //    }
        //}
        if (!(msg->data == NULL || !cache_line->valid))
        {
            writeCacheLineData(data_ptr);
            return;
        }
        // If we're invalidating the line, message over?
        //else if (cache_line->valid == false)
        //{
        //    std::cerr << msg->msg_id << "," << msg->addr << "," 
        //    << "wrCache" << "," <<  m_core_id << ","<< m_cache_cycle << "\n";
        //    std::cerr << msg->msg_id << "," << msg->addr << "," 
        //    << "termina" << "," <<  m_core_id << ","<< m_cache_cycle<<"\n";
        //}

        cache_line->~GenericCacheLine();    //explicit call for the destructor due to the use of placement new
        msg->~Message();                    //explicit call for the destructor due to the use of placement new

        delete[] (uint8_t *)data_ptr;
    }
    
    void CacheController::writeCacheLineData(void *data_ptr)
    {
        Message *msg = (Message *)data_ptr;
        GenericCacheLine *cache_line = (GenericCacheLine *)((uint8_t *)data_ptr + sizeof(Message));

        if(!checkReadinessOfCache(*msg, ControllerAction::Type::WRITE_CACHE_LINE_DATA, 
            data_ptr)) {
            std::cerr << msg->msg_id << "," << msg->addr << "," \
                    << "datNrdy" << "," <<  m_core_id << ","<< m_cache_cycle << "\n";
            return;
        }

        std::cerr << msg->msg_id << "," << msg->addr << "," \
                << "updaDat" << "," <<  m_core_id << ","<< m_cache_cycle << "\n";

        if (!m_data_handler->updateLineData(msg->addr, msg->data, m_protocol,
            msg->msg_id, m_core_id))
        {
            cout << "CacheController: update data of an unfound line" << endl;
            exit(0);
        }
        cache_line->~GenericCacheLine();    //explicit call for the destructor due to the use of placement new
        msg->~Message();                    //explicit call for the destructor due to the use of placement new

        delete[] (uint8_t *)data_ptr;
    }
    
    void CacheController::saveReqForWriteBack(void *data_ptr)
    {
        Message *msg = (Message *)data_ptr;
        std::cerr << msg->msg_id << "," << msg->addr << "," 
                << "writeBk" << "," <<  m_core_id << ","<< -2 << "\n";
        this->m_saved_requests_for_wb[this->getAddressKey(msg->addr)] = *msg;

        delete msg;
    }

    void CacheController::stall(void *data_ptr)
    {
        Message *msg = (Message *)data_ptr;
        
        if (!m_processing_queue->pushBack(*msg, FRFCFS_State::NonReady))
        {
            cout << "CacheController: error there is no free space to push request to processing queue" << endl;
            exit(0);
        }
        delete msg;
    }

    void CacheController::initializeCacheData(std::vector<std::string> &tracePaths)
    {
        uint64_t mockup_data = 1;
        for (string path : tracePaths)
        {
            ifstream file(path);
            string line;

            if (!file.is_open())
            {
                cout << "ERROR: Can't open trace file" << endl;
                exit(0);
            }

            while (getline(file, line))
            {
                unsigned long long address;

                sscanf(line.c_str(), "%llx", &address);

                if (!this->m_data_handler->readLineBits(address))
                {

                    if (m_data_handler->findEmptyWay(address) == -1)
                    {
                        cout << "ERROR: Cache is not prefect ... increase the cache size" << endl;
                        exit(0);
                    }

                    GenericCacheLine cache_line;
                    this->m_protocol->createDefaultCacheLine(address, &cache_line);
                    memcpy(cache_line.m_data, &mockup_data, sizeof(mockup_data));

                    this->m_data_handler->writeCacheLine_bypassLatency(address, &cache_line);
                    mockup_data++;
                }
            }
            file.close();
        }
    }

    bool CacheController::checkReadinessOfCache(Message &msg, ControllerAction::Type type, void *data_ptr)
    {
        if(!m_data_handler->isReady(msg.addr))
        {
            m_data_access_buffer.push_back(msg);
            m_data_access_action[msg.msg_id] = ControllerAction{.type = type,
                                                               .data = data_ptr};
            return false;
        }
        return true;
    }

    void CacheController::checkReplacements(FRFCFS_Buffer<Message, CoherenceProtocolHandler> &buf)
    {
        uint64_t evicted_address = 0;
        Message msg;
        // Check if a line has recently been added to WB and get its address
        if (((CacheDataHandler_COTS*)m_data_handler)->addressOfLinePendingWB(false, &evicted_address))
        {
            // If so, generate a new REPLACE message with its address
            msg = Message(IdGenerator::nextReqId(),            // Id
                          evicted_address,                     // Addr
                          m_cache_cycle,                       // Cycle
                          (uint64_t)CpuFIFO::REQTYPE::REPLACE, // Complementary_value
                          (uint16_t)this->m_core_id);          // Owner
            msg.to.push_back((uint16_t)this->m_shared_memory_id);

            // Push the REPLACE message onto the message queue as NonReady
            // If successful, unset the flag that shows an entry was
            // recently sent to WB
            std::cerr << msg.msg_id << "," << msg.addr << "," \
                << "add2q_r" << "," <<  m_core_id << ","<< m_cache_cycle<<"\n";

            if (buf.pushBack(msg, FRFCFS_State::NonReady))
            {
                ((CacheDataHandler_COTS*)m_data_handler)->addressOfLinePendingWB(true, &evicted_address);
            }
            else 
            {
                std::cerr << msg.msg_id << "," << msg.addr << "," \
                    << "termina" << "," <<  m_core_id << ","<< m_cache_cycle<<"\n";
                std::cerr << msg.msg_id << "," << msg.addr << "," \
                    << "wrCache" << "," << 99 << ","<< -1 <<"\n";
                std::cerr << msg.msg_id << "," << msg.addr << "," \
                    << "wrCache" << "," << 10 << ","<< -1 <<"\n";
            }
            // loop through the data access buffer, calling the corresponding
            // action functions
            for(int i = 0; i < (int)m_data_access_buffer.size(); )
            {
                if(getAddressKey(m_data_access_buffer[i].addr) == getAddressKey(evicted_address))
                {
                    callActionFunction(m_data_access_action[m_data_access_buffer[i].msg_id]);
                    m_data_access_action.erase(m_data_access_buffer[i].msg_id);  //after erasing the looping counter shouldn't get incremented
                    m_data_access_buffer.erase(m_data_access_buffer.begin() + i);
                }
                else
                    i++;
            }
        }
    }
}
