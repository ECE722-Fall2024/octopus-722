/*
 * File  :      RRFCFSArbiter.h
 * Author:      Mohammed Ismail
 * Email :      ismaim22@mcmaster.ca
 *
 * Created On April 21, 2022
 */

#ifndef _RRFCFS_ARBITER_H
#define _RRFCFS_ARBITER_H

#include "Arbiter.h"

using namespace std;

namespace ns3
{
    class RRFCFSArbiter: public Arbiter
    {
    protected:       
        struct MsgData {
            Message msg;
            int core_index;
            int msg_index;
        };
        uint64_t candidate_id;
        uint64_t prev_candidate_id = 0;

        virtual uint64_t selectCandidate(uint64_t cycle_number);
        virtual bool coreElect(vector<vector<Message>*>& buffers, Message *out_msg);
        virtual int electFCFS(vector<struct MsgData> &buffer, Message *out_msg);

    public:
        RRFCFSArbiter(vector<int>* candidates_ids, int arbiter_period);
        ~RRFCFSArbiter();

        virtual bool elect(uint64_t cycle_number, vector<vector<Message>*>& buffers, Message *out_msg) override;
        virtual bool forceElect(uint64_t cycle_number, vector<vector<Message>*>& buffers, Message *out_msg) override;
        virtual bool checkNext(uint64_t cycle_number, vector<vector<Message>*>& buffers, int bus_type, int latency);
    };
}

#endif /* _RRFCFS_ARBITER_H */
