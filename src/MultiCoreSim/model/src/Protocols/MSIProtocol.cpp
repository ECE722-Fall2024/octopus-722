/*
 * File  :      MSIProtocol.cpp
 * Author:      Mohammed Ismail
 * Email :      ismaim22@mcmaster.ca
 *
 * Created On June 23, 2021
 */

#include "../../header/Protocols/MSIProtocol.h"
#include "../alloc_setting.h"
using namespace std;

namespace ns3
{
    MSIProtocol::MSIProtocol(CacheDataHandler *cache, const string &fsm_path, int coreId, int sharedMemId) : CoherenceProtocolHandler(cache, fsm_path, coreId, sharedMemId)
    {
    }

    MSIProtocol::~MSIProtocol()
    {
    }

    FRFCFS_State MSIProtocol::getRequestState(const Message &msg, FRFCFS_State req_state)
    {
        GenericCacheLine cache_line;
        m_data_handler->readLineBits(msg.addr, &cache_line);
        // Check if the requested cache line is not currently in cache
        if (cache_line.valid == false)
        {
            std::cerr << msg.msg_id << "," << msg.addr << "," \
                << "Miss?  " << "," <<  m_core_id << ","<< 1 << "\n";
            // Find an empty way to bring the cache line into
            bool is_space = m_data_handler->findSpace(msg.addr);
            // If there is no empty way then we need to do a replacement
            if (!is_space && req_state == FRFCFS_State::NonReady)
            {
                // TODO: Is this line good? If msg is trying to replace a line
                // that already doesn't exist, no need to start replacement
                //if (msg.complementary_value == 2) return FRFCFS_State::Ready;
                // attempt to begin allocation process
                if (m_data_handler->freeUpSpace(msg, this))
                {
                    // if success, set state to waiting
                    return FRFCFS_State::Waiting;
                }
                else
                {
                    // if failure, keep NonReady to try again next cycle
                    // this happens because WB is full (alloc-on-miss)
                    // of because MSHR was previously found to be full (refill)
                    std::cerr << msg.msg_id << "," << msg.addr << "," \
                    << "MSInrdy" << "," <<  m_core_id << ","<< m_cycle << "\n";

                    return FRFCFS_State::NonReady;
                }
            }
            else if (!is_space && req_state == FRFCFS_State::Waiting)
            {
                // We are already in the replacement process, so just wait
                return FRFCFS_State::Waiting;
            }
            else
            {
                std::cerr << msg.msg_id << "," << msg.addr << "," \
                    << "MSIredy" << "," <<  m_core_id << ","<< m_cycle << "\n";
                return FRFCFS_State::Ready;
            }
        }
        // This will protect allocated lines from being evicted if they
        // have not yet received their data (i.e. transient state)
        if (this->m_fsm->isStall(cache_line.state, msg.complementary_value))
            return FRFCFS_State::NonReady;

        return FRFCFS_State::Ready;
    }

    std::vector<int> MSIProtocol::statesRequireWriteBack()
    {
        vector<int> states;
        states.push_back(this->m_fsm->getState(string("M")));
        return states;
    }

    // Based on the request message and the current state of the address'
    // cache line, determine the next state of the line and return the 
    // required actions
    const vector<ControllerAction> &MSIProtocol::processRequest(Message &request_msg)
    {
        EventId event_id;
        int next_state;
        vector<int> actions;
        GenericCacheLine cache_line;

        m_data_handler->readLineBits(request_msg.addr, &cache_line);

        this->readEvent(request_msg, &event_id);
        this->m_fsm->getTransition(cache_line.state, (int)event_id, next_state, actions);

        // Timestamp code
        // Message issued from core to its L1:
        if (request_msg.source == Message::Source::LOWER_INTERCONNECT)
        {
            // If the message is a load or store:
            // check if message is a hit; if so, no need to modify either cache
            if ((event_id == EventId::Load || event_id == EventId::Store) &&
                actions == vector<int>{1})
            {
                std::cerr << request_msg.msg_id << "," << request_msg.addr 
                    << "," << "wrCache" << "," <<  m_core_id << ","<< -1 << "\n";
                std::cerr << request_msg.msg_id << "," << request_msg.addr 
                    << "," << "wrCache" << "," <<  10 << ","<< -1 << "\n";
                std::cerr << request_msg.msg_id << "," << request_msg.addr 
                    << "," << "hitActn" << "," <<  m_core_id << ","<< -2 << "\n";
                std::cerr << request_msg.msg_id << "," << request_msg.addr 
                    << "," << "termina" << "," <<  m_core_id << ","<< m_cycle << "\n";
            }
            // If the message is a replacement, L1's data will not be modified:
            if (event_id == EventId::Replacement)
            {
                auto isPutM = std::find(actions.begin(), actions.end(), 4);
                std::cerr << request_msg.msg_id << "," << request_msg.addr 
                    << "," << "wrCache" << "," <<  m_core_id << ","<< -1 << "\n";
                std::cerr << request_msg.msg_id << "," << request_msg.addr 
                    << "," << "termina" << "," <<  m_core_id << ","<< m_cycle << "\n";

                // If no PutM upon replacement, msg won't modify LLC
                if (isPutM == actions.end())
                {
                    std::cerr << request_msg.msg_id << "," << request_msg.addr 
                        << "," << "wrCache" << "," <<  10 << ","<< -1 << "\n";
                }
            }
        }

        // L1's request message seen on bus by other cache controller
        if (request_msg.source == Message::Source::UPPER_INTERCONNECT &&
            (event_id == EventId::Other_GetM || event_id == EventId::Other_GetS))
        {
            //auto isData2Req = std::find(actions.begin(), actions.end(), 5);
            //auto isSaveReq = std::find(actions.begin(), actions.end(), 7);
            //// If the controller does a SaveReq, then this message will
            //// depend on data sent from another L1---i.e. it won't modfy LLC
            //if (isSaveReq != actions.end() || isData2Req != actions.end())
            //{
            //    std::cerr << request_msg.msg_id << "," << request_msg.addr 
            //        << "," << "wrCache" << "," <<  10 << ","<< -1 << "\n";
            //}
        }

        if (event_id == EventId::OwnData)
        {
            //auto isData2Req = std::find(actions.begin(), actions.end(), 5);
            //auto isData2Both = std::find(actions.begin(), actions.end(), 6);
            if (next_state == 0)
            {
                std::cerr << request_msg.msg_id << "," << request_msg.addr 
                    << "," << "respond" << "," <<  m_core_id << ","<< -2 << "\n";
                std::cerr << request_msg.msg_id << "," << request_msg.addr 
                    << "," << "wrCache" << "," <<  m_core_id << ","<< -1 << "\n";
                std::cerr << request_msg.msg_id << "," << request_msg.addr 
                    << "," << "termina" << "," <<  m_core_id << ","
                    << m_cycle << "\n";
            }
        }

        // If invalidation request, the msg isn't going to update L1 data
        if (event_id == EventId::Invalidation)
        {
            std::cerr << request_msg.msg_id << "," << request_msg.addr 
                    << "," << "wrCache" << "," <<  m_core_id << ","<< -1 << "\n";
            // If the invalidation doesn't have an effect, the message is over
            //if (cache_line.state == next_state && cache_line.state != 0)
            //{
            //    std::cerr << request_msg.msg_id << "," << request_msg.addr 
            //        << "," << "wrCache" << "," <<  10 << ","<< -1 << "\n";
            //    std::cerr << request_msg.msg_id << "," << request_msg.addr 
            //        << "," << "termina" << "," <<  m_core_id << ","<< m_cycle << "\n";
            //}
            if (cache_line.state != next_state && next_state == 0)
            {
                std::cerr << request_msg.msg_id << "," << request_msg.addr 
                    << "," << "termina" << "," <<  m_core_id << ","<< m_cycle << "\n";
            }
            else if (cache_line.state == 19 || cache_line.state == 9)
            {
                std::cerr << request_msg.msg_id << "," << request_msg.addr 
                    << "," << "wrCache" << "," << 10 << ","<< -1 << "\n";
                std::cerr << request_msg.msg_id << "," << request_msg.addr 
                    << "," << "termina" << "," <<  m_core_id << ","<< m_cycle << "\n";
            }
        }

        return handleAction(actions, request_msg, cache_line, next_state);
    }

    vector<ControllerAction> &MSIProtocol::handleAction(std::vector<int> &actions, Message &msg,
                                                        GenericCacheLine &cache_line, int next_state)
    {
        this->controller_actions.clear();
        for (int action : actions)
        {
            ControllerAction controller_action;
            switch (static_cast<ActionId>(action))
            {
            case ActionId::Stall:
                controller_action.type = ControllerAction::Type::STALL;
                controller_action.data = (void *)new Message();
                ((Message *)controller_action.data)->copy(msg);
                // std::cout << " MSIProtocol: Stall Transaction is detected" << std::endl;
                // exit(0);
                break;

            case ActionId::Hit: // remove request from pending and respond to cpu, update cache line
                controller_action.type = (msg.source == Message::Source::LOWER_INTERCONNECT)
                                             ? ControllerAction::Type::HIT_Action
                                             : ControllerAction::Type::REMOVE_PENDING;
                controller_action.data = (void *)new Message();
                ((Message *)controller_action.data)->copy(msg);
                break;

            case ActionId::GetS:
            case ActionId::GetM:
                // add request to pending requests
                controller_action.type = ControllerAction::Type::ADD_PENDING;
                controller_action.data = (void *)new Message();
                ((Message *)controller_action.data)->copy(msg);
                this->controller_actions.push_back(controller_action);

                // send Bus request, update cache line
                controller_action.type = ControllerAction::Type::SEND_BUS_MSG;
                controller_action.data = (void *)new Message(msg.msg_id, // Id
                                                             msg.addr,   // Addr
                                                             0,          // Cycle
                                                             (action == (int)ActionId::GetS) ? MSIProtocol::REQUEST_TYPE_GETS
                                                                                             : MSIProtocol::REQUEST_TYPE_GETM, // Complementary_value
                                                             (uint16_t)this->m_core_id);                                       // Owner

                ((Message *)controller_action.data)->to.push_back((uint16_t)this->m_shared_memory_id);
                break;
            case ActionId::PutM:
                // send Bus request, update cache line
                controller_action.type = ControllerAction::Type::SEND_BUS_MSG;
                controller_action.data = (void *)new Message(msg.msg_id,                     // Id
                                                             msg.addr,                       // Addr
                                                             0,                              // Cycle
                                                             MSIProtocol::REQUEST_TYPE_PUTM, // Complementary_value
                                                             (uint16_t)this->m_core_id);     // Owner
                ((Message *)controller_action.data)->to.push_back((uint16_t)this->m_shared_memory_id);
                break;

            case ActionId::Data2Req:
            case ActionId::Data2Both:
                // Do writeback, update cache line
                controller_action.type = ControllerAction::Type::WRITE_BACK;
                controller_action.data = (void *)new Message(msg);

                ((Message *)controller_action.data)->to.clear();
                if (action == (int)ActionId::Data2Both)
                    ((Message *)controller_action.data)->to.push_back((uint16_t)this->m_shared_memory_id);
                break;

            case ActionId::SaveReq:
                // send Bus request, update cache line
                controller_action.type = ControllerAction::Type::SAVE_REQ_FOR_WRITE_BACK;
                controller_action.data = (void *)new Message(msg.msg_id, // Id
                                                             msg.addr,   // Addr
                                                             0,          // Cycle
                                                             0,          // Complementary_value
                                                             msg.owner); // Owner
                break;

            case ActionId::Fault:
                std::cout << " MSIProtocol: Fault Transaction is detected" << std::endl;
                assert(1 != 1);
//                exit(0);
                break;
            }

            this->controller_actions.push_back(controller_action);
        }

        if ((actions.size() > 0) && (actions[0] == (int)ActionId::Stall))
            return this->controller_actions;

        // update cache line
        ControllerAction controller_action;

        cache_line.valid = this->m_fsm->isValidState(next_state);
        cache_line.state = next_state;

        controller_action.type = ControllerAction::Type::UPDATE_CACHE_LINE;
        controller_action.data = (void *)new uint8_t[sizeof(Message) + sizeof(cache_line)];

        new (controller_action.data) Message(msg);
        new ((uint8_t *)controller_action.data + sizeof(Message)) GenericCacheLine(cache_line);

        this->controller_actions.push_back(controller_action);

        return this->controller_actions;
    }

    void MSIProtocol::readEvent(Message &msg, EventId *out_id)
    {
        switch (msg.source)
        {
        case Message::Source::LOWER_INTERCONNECT:
            *out_id = (msg.complementary_value == CpuFIFO::REQTYPE::READ) ? EventId::Load : (msg.complementary_value == CpuFIFO::REQTYPE::WRITE) ? EventId::Store
                                                                                                                                                 : EventId::Replacement;
//            switch (*out_id)
//            {
//            case EventId::Load:
//                std::cout << "Load\n"; break;
//            case EventId::Store:
//                std::cout << "Store\n"; break;
//            case EventId::Replacement:
//                std::cout << "Replacement\n"; break;
//            }
            return;

        case Message::Source::UPPER_INTERCONNECT:
            if (msg.data != NULL)
                *out_id = EventId::OwnData;

            else
            {
                switch (msg.complementary_value)
                {
                case MSIProtocol::REQUEST_TYPE_GETS:
                    *out_id = (msg.owner == m_core_id) ? EventId::Own_GetS : EventId::Other_GetS;
                    return;
                case MSIProtocol::REQUEST_TYPE_GETM:
                    *out_id = (msg.owner == m_core_id) ? EventId::Own_GetM : EventId::Other_GetM;
                    return;
                case MSIProtocol::REQUEST_TYPE_PUTM:
                    *out_id = (msg.owner == m_core_id) ? EventId::Own_PutM : EventId::Other_PutM;
                    return;
                case MSIProtocol::REQUEST_TYPE_INV:
                    *out_id = EventId::Invalidation;
                    return;
                default: // Invalid Transaction
                    std::cout << " MSIProtocol: Invalid Transaction detected on the Bus" << std::endl;
                    exit(0);
                }
            }
            return;

        default:
            std::cout << "Invalid message source" << std::endl;
        }
    }
}
