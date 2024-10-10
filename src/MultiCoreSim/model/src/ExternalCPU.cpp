/*
 * File  :      ExternalCPU.cpp
 * Author:      Mohammed Ismail
 * Email :      ismaim22@mcmaster.ca
 *
 * Created On Sept 6, 2022
 */

#include "../header/ExternalCPU.h"

namespace ns3
{
    // override ns3 type
    TypeId ExternalCPU::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::ExternalCPU").SetParent<Object>();
        return tid;
    }

    // private controller constructor
    ExternalCPU::ExternalCPU(CacheXml &xml, CommunicationInterface *upper_interface)
    {
        m_id = xml.GetCacheId();

        m_dt = xml.GetCpuClkNanoSec();
        m_clk_skew = xml.GetCpuClkSkew();
        m_clk_cycle = 1;

        m_upper_interface = upper_interface;

        m_processing_queue = new FRFCFS_Buffer<Message, ExternalCPU>(&ExternalCPU::getRequestState, this);
        
        m_cpu_callback = NULL;
    }

    ExternalCPU::~ExternalCPU()
    {
    }

    void ExternalCPU::cycleProcess()
    {
        processLogic();
        Simulator::Schedule(NanoSeconds(m_dt), &ExternalCPU::step, Ptr<ExternalCPU>(this));
        m_clk_cycle++;
    }

    void ExternalCPU::init()
    {
        Simulator::Schedule(NanoSeconds(m_clk_skew), &ExternalCPU::step, Ptr<ExternalCPU>(this));
    }

    void ExternalCPU::step(Ptr<ExternalCPU> ext_cpu)
    {
        ext_cpu->cycleProcess();
    }

    void ExternalCPU::processLogic()
    {
        addRequests2ProcessingQueue(*m_processing_queue);

        Message ready_msg;
        while (true)
        {
            if (m_processing_queue->getFirstReady(&ready_msg) == false)
                return;

            if(ready_msg.source == Message::Source::LOWER_INTERCONNECT)
            {
                ready_msg.cycle = this->m_clk_cycle;
                if (!m_upper_interface->pushMessage(ready_msg, m_clk_cycle, MessageType::REQUEST))
                {
                    cout << "ExternalCPU(id = " << this->m_id << "): ";
                    cout << "Cannot insert the Msg into the upper interface FIFO, FIFO is Full!" << endl;
                    exit(0);
                }
            }
            else if(ready_msg.source == Message::Source::UPPER_INTERCONNECT)
            {
                if(m_cpu_callback != NULL)
                    (*m_cpu_callback)(ready_msg.addr, this->m_clk_cycle, (RequestType)ready_msg.complementary_value);
                else
                {
                    cout << "ExternalCPU(id = " << this->m_id << "): ";
                    cout << "Error CPU Callback is invalid" << endl;
                    exit(0);
                }
            }
        }
    }

    void ExternalCPU::addRequests2ProcessingQueue(FRFCFS_Buffer<Message, ExternalCPU> &buf)
    {
        Message msg;

        if (m_upper_interface->peekMessage(&msg))
        {
            msg.source = Message::Source::UPPER_INTERCONNECT;
            msg.cycle = m_clk_cycle;
            if (buf.pushBack(msg))
                m_upper_interface->popFrontMessage();
        }
    }

    FRFCFS_State ExternalCPU::getRequestState(const Message &msg, FRFCFS_State current_state)
    {
        return FRFCFS_State::Ready;
    }

    void ExternalCPU::registerCPUCallback(CallbackGeneral<uint64_t, uint64_t, RequestType>* cpu_callback)
    {
        this->m_cpu_callback = cpu_callback;
    }

    void ExternalCPU::addRequest(uint64_t address, RequestType type)
    {
        Message request_msg(IdGenerator::nextReqId(), // id
                            address,                  // Addr
                            0,                        // Cycle
                            (uint64_t)type,           // Complementary_value
                            this->m_id);              // Owner
        request_msg.source = Message::Source::LOWER_INTERCONNECT;
        m_processing_queue->pushBack(request_msg);
    }

    map<int, ExternalCPU*> ExternalCPU::ext_CPUs;
    
    map<int, ExternalCPU*>* ExternalCPU::getExtCPUs()
    {
        return &ExternalCPU::ext_CPUs;
    }
}