/*
 * File  :      RRFCFSArbiter.cpp
 * Author:      Mohamed Hassan
 * Email :      mohamed.hassan@mcmaster.ca
 *
 * Created On August 29, 2023
 */

#include "../../header/Arbiters/RRFCFSArbiter.h"
#include <iostream>

namespace ns3
{
    RRFCFSArbiter::RRFCFSArbiter(vector<int> *candidates_ids, int arbiter_period) : Arbiter(candidates_ids, arbiter_period)
    {
        candidate_id = 0;
    }

    RRFCFSArbiter::~RRFCFSArbiter()
    {
    }

 

    bool RRFCFSArbiter::coreElect(vector<vector<Message> *> &buffers, Message *out_msg)
    {
        /* construct a buffer of the messages of that core ID*/
        vector<struct MsgData> coreBuffer;
        //vector<Message> coreBuffer;
        Message coreMsg;
        // Loop through each core's buffer, adding each buffer's oldest message
        // to a linear queue. 
        for (int i = 0; i < (int)buffers.size(); i++)
        {
            // Attempt to find message in queue. If exists, add to buffer
            int msg_index = findMessage(*buffers[i], candidate_id);
            if (msg_index != -1)
            {
                coreMsg.copy(buffers[i]->at(msg_index));
                struct MsgData msg_data = {coreMsg, i, msg_index};
                coreBuffer.push_back(msg_data);
            }
        }

        /* now we do FCFS of the messages on the coreBuffer*/
        int coreBufferIndex = electFCFS(coreBuffer, out_msg);
        if (coreBufferIndex != -1)
        {
            int coreIndex = coreBuffer[coreBufferIndex].core_index;
            int msgIndex = coreBuffer[coreBufferIndex].msg_index;
            buffers[coreIndex]->erase(buffers[coreIndex]->begin() + msgIndex);
            return true;
        }

        return false;
    }

    bool RRFCFSArbiter::checkNext(uint64_t cycle_number, vector<vector<Message> *> &buffers, 
        int bus_type, int latency)
    {
        // The candidate is the same as the candidate chosen when elect()
        // was called, so we can reuse that value
        /* construct a buffer of the messages of that core ID*/
        vector<struct MsgData> coreBuffer;
        //vector<Message> coreBuffer;
        Message coreMsg;
        Message outMsg = Message();
        Message * out_msg = &outMsg;
        // Loop through each core's buffer, adding each buffer's oldest message
        // to a linear queue. 
        for (int i = 0; i < (int)buffers.size(); i++)
        {
            // Attempt to find message in queue. If exists, add to buffer
            int msg_index = findMessage(*buffers[i], prev_candidate_id);
            if (msg_index != -1)
            {
                coreMsg.copy(buffers[i]->at(msg_index));
                struct MsgData msg_data = {coreMsg, i, msg_index};
                coreBuffer.push_back(msg_data);
            }
        }

        /* now we do FCFS of the messages on the coreBuffer*/
        int coreBufferIndex = electFCFS(coreBuffer, out_msg);
        if (coreBufferIndex != -1)
        {
            int coreIndex = coreBuffer[coreBufferIndex].core_index;
            // convert core index to core id
            int core_id;
            switch (coreIndex)
            {
                case 0:
                core_id = 10; break;
                default:
                core_id = coreIndex - 1;
            }
            // Print timestamp
            if (bus_type == 0)
            {
                cerr << out_msg->msg_id << "," << out_msg->addr << ","
                    << "add_req"
                    << "," <<  core_id << ","<< (cycle_number)<<"\n";
            }
            if (bus_type == 1)
            {
                cerr << out_msg->msg_id << "," << out_msg->addr << ","
                    << ((core_id == 10) ? "respond" : "writeBk")
                    << "," <<  core_id << ","<< (cycle_number)<<"\n";
            }
            return true;
        }

        return false;

    }

    int RRFCFSArbiter::electFCFS(vector<struct MsgData> &buffer, Message *out_msg)
    {
        uint64_t min_cycle = 0xFFFFFFFFFFFFFFFF;
        int min_buffer_index = -1;

        for (int i = 0; i < (int)buffer.size(); i++)
        {
            if (buffer[i].msg.cycle < min_cycle)
            {
                min_buffer_index = i;
                min_cycle = buffer[i].msg.cycle;
            }
        }

        if (min_buffer_index != -1)
        {
            out_msg->copy(buffer[min_buffer_index].msg);
            //buffer.erase (buffer.begin()+min_buffer_index);
            return min_buffer_index;
        }

        return -1;
    }


    bool RRFCFSArbiter::elect(uint64_t cycle_number, vector<vector<Message> *> &buffers, Message *out_msg)
    {
        for (int i = 0; i < (int)m_candidates_ids->size(); i++)
        {
            candidate_id = selectCandidate(cycle_number);
            if (coreElect(buffers, out_msg))
            {
                prev_candidate_id = candidate_id;
                return true;
            }
        }
        return false;
    }

    bool RRFCFSArbiter::forceElect(uint64_t cycle_number, vector<vector<Message> *> &buffers, Message *out_msg)
    {
        return coreElect(buffers, out_msg);
    }

    uint64_t RRFCFSArbiter::selectCandidate(uint64_t cycle_number)
    {
        candidate_index = (candidate_index + 1) % m_candidates_ids->size();

        return m_candidates_ids->at(candidate_index);
    }
}