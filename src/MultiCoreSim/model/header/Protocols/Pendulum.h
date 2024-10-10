/*
 * File  :      Pendulum.h
 * Author:      Safin Bayes
 * Email :      bayess@mcmaster.ca
 *
 * Created On March 02, 2022
 */

#ifndef _Pendulum_H
#define _Pendulum_H

#include "MSIProtocol.h"

namespace ns3
{
    class Pendulum: public MSIProtocol
    {
    protected:
        enum class EventId
        {
            Load = 0,
            Store,
            Replacement,

            Own_GetS,
            Own_GetM,
            Own_PutM,
            
            Other_GetS,
            Other_GetM,
            
            Timeout,
            
            Own_InvGetM,
            Own_SelfInv,
            
            Other_InvGetM,
            
            Data,
        };

        enum class ActionId
        {
            Stall = 0,
            Hit,
            GetS,
            GetM,
            PutM,
            SelfInv,
            SaveReq,
            Data2Req,
            Data2Both,
            Inv_GetM,
            Stop_T,
            RT,
            Fault
        };

        virtual void readEvent(Message &msg, MSIProtocol::EventId *out_id) override;
        virtual std::vector<ControllerAction> &handleAction(std::vector<int> &actions, Message &msg,
                                                            const GenericCache::CacheLineInfo &cache_line_info, int next_state) override;

    public:
        Pendulum(GenericCache *cache, const std::string &fsm_path, int coreId, int sharedMemId);
        ~Pendulum();

    };
}

#endif
