/*
 * File  :      DirectInterconnect.h
 * Author:      Mohammed Ismail
 * Email :      ismaim22@mcmaster.ca
 *
 * Created On April 5, 2022
 */

#ifndef _DIRECT_INTERCONNECT_H
#define _DIRECT_INTERCONNECT_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/core-module.h"

#include "ns3/CommunicationInterface.h"
#include "DirectController.h"

#include <list>
#include <vector>
#include <map>

using namespace std;

namespace ns3
{
    class DirectInterconnect : public ns3::Object
    {
    protected:
        vector<CommunicationInterface *> m_interfaces;
        map<int, vector<int>> m_topology;

        DirectController* interconnect_controller;

        int m_interconnect_cycle;

    public:
        static TypeId GetTypeId(void); // Override TypeId.

        DirectInterconnect(int lower_id, int upper_id, int buffers_max_size);
        ~DirectInterconnect();

        void cycleProcess();

        CommunicationInterface* getInterfaceFor(int id);
        
        virtual void init();
        static void step(Ptr<DirectInterconnect> bus);
    };
}

#endif /* _DIRECT_INTERCONNECT_H */
