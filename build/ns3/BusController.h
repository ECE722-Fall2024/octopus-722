/*
 * File  :      BusController.h
 * Author:      Mohammed Ismail
 * Email :      ismaim22@mcmaster.ca
 *
 * Created On April 10, 2022
 */

#ifndef _BUS_CONTROLLER_H
#define _BUS_CONTROLLER_H

#include "BusInterface.h"

#include "ns3/CommunicationInterface.h"

#include "ns3/Arbiter.h"
#include "ns3/TDMArbiter.h"
#include "ns3/FCFSArbiter.h"
#include "ns3/RRArbiter.h"
#include "ns3/RRFCFSArbiter.h"

#include "ns3/Logger.h"

#include <vector>
#include <map>

using namespace std;

namespace ns3
{
    class BusController
    {
    public:
        enum class BusType
        {
            RequestBus = 0,
            ResponseBus,
        };

    protected:
        vector<CommunicationInterface *> *m_interfaces;
        vector<int> *m_lower_level_ids;
        vector<Arbiter *> m_arbiters;

        int m_request_latency;
        int m_response_latency;

        virtual void broadcast(Message &msg, MessageType type = MessageType::REQUEST);
        virtual void send(Message &msg, MessageType type = MessageType::DATA_RESPONSE);

    public:
        BusController(vector<CommunicationInterface *> *interfaces, 
            vector<int> *lower_level_ids, int req_delay = 2, int resp_delay = 5);
        ~BusController();

        virtual void busStep(uint64_t cycle_number){};
    };
}

#endif /* _BUS_CONTROLLER_H */
