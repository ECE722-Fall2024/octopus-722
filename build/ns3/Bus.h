/*
 * File  :      Bus.h
 * Author:      Mohammed Ismail
 * Email :      ismaim22@mcmaster.ca
 *
 * Created On April 5, 2022
 */

#ifndef _BUS_H
#define _BUS_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/core-module.h"

#include "ns3/CacheXml.h"
#include "ns3/CommunicationInterface.h"
#include "BusController.h"
#include "SplitBusController.h"
#include "UnifiedBusController.h"
#include "Point2PointController.h"

#include <list>
#include <vector>
#include <map>

using namespace std;

namespace ns3
{
    class Bus : public ns3::Object
    {
    protected:
        vector<CommunicationInterface *> m_interfaces;
        vector<int> m_lower_level_ids;
        map<int, vector<int>> m_topology;

        BusController* interconnect_controller;

        int m_bus_cycle;
        int m_bus_cycle_edges;

    public:
        static TypeId GetTypeId(void); // Override TypeId.

        Bus();
        Bus(list<CacheXml>& lower_level_caches, list<CacheXml>& upper_level_caches, int buffers_max_size);
        Bus(list<CacheXml> &lower_level_caches, int upper_level_id, int buffers_max_size, vector<int>* candidates_id = NULL);
        ~Bus();

        void cycleProcess();

        CommunicationInterface* getInterfaceFor(int id);

        vector<int> *getLowerLevelIds();
        
        virtual void init();
        static void step(Ptr<Bus> bus);
    };
}

#endif /* _BUS_H */
