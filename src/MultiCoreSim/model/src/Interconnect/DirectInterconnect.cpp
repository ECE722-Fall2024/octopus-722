/*
 * File  :      DirectInterconnect.cpp
 * Author:      Mohammed Ismail
 * Email :      ismaim22@mcmaster.ca
 *
 * Created On Sept 15, 2022
 */

#include "../../header/Interconnect/DirectInterconnect.h"

namespace ns3
{
    TypeId DirectInterconnect::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::DirectInterconnect").SetParent<Object>();
        return tid;
    }

    DirectInterconnect::DirectInterconnect(int lower_id, int upper_id, int buffers_max_size)
    {
        m_interconnect_cycle = 1;

        m_interfaces.push_back(new BusInterface(lower_id, buffers_max_size));
        m_interfaces.push_back(new BusInterface(upper_id, buffers_max_size));

        for (int i = 0; i < (int)m_interfaces.size(); i++)
        {
            m_topology[m_interfaces[i]->m_interface_id] = vector<int>();
            for (int j = 0; j < (int)m_interfaces.size(); j++)
            {
                if (j == i)
                    continue;
                m_topology[m_interfaces[i]->m_interface_id].push_back(m_interfaces[j]->m_interface_id);
            }
        }
        
        interconnect_controller = new DirectController(&m_interfaces);
    }

    DirectInterconnect::~DirectInterconnect()
    {
        for (int i = 0; i < (int)m_interfaces.size(); i++)
        {
            delete m_interfaces[i];
        }
        m_interfaces.clear();
    }
    
    CommunicationInterface* DirectInterconnect::getInterfaceFor(int id)
    {
        for(int i = 0; i < (int)m_interfaces.size(); i++)
        {
            if(m_interfaces[i]->m_interface_id == id)
                return m_interfaces[i];
        }
        return NULL;
    }
    
    void DirectInterconnect::cycleProcess()
    {
        interconnect_controller->controllerStep(m_interconnect_cycle++);
        Simulator::Schedule(NanoSeconds(100), &DirectInterconnect::step, Ptr<DirectInterconnect>(this)); // Schedule the next run
    }

    void DirectInterconnect::init()
    {
        Simulator::Schedule(NanoSeconds(0), &DirectInterconnect::step, Ptr<DirectInterconnect>(this));
    }

    void DirectInterconnect::step(Ptr<DirectInterconnect> DirectInterconnect)
    {
        DirectInterconnect->cycleProcess();
    }
}