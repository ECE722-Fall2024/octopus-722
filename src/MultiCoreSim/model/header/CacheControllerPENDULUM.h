/*
 * File  :      CacheControllerPEN.h
 * Author:      Safin Bayes
 * Email :      bayess@@mcmaster.ca
 *
 * Created On March 09, 2022
 */

#ifndef _CacheControllerPEN_H
#define _CacheControllerPEN_H

#include "CacheController.h"
#include "TimerCache.h"

namespace ns3
{
    class CacheControllerPENDULUM : public CacheController
    {
        protected:
            uint64_t timer_val;     // TODO: Should come from the user
            int sharer_count;
            uint32_t num_cacheLines; 
            // TimerCache* m_cache_pendulum; 
            std::vector<Message> self_msgs;
            
            virtual void calc_timeout();
            virtual void callActionFunction(ControllerAction) override;      //Why no override
            
            virtual void timer_actions(void *);
            virtual void add_sharer(void *);                  // For LLC
            virtual void remove_sharer(void *);               // For LLC
            
            virtual void initializeCacheHandler(CacheXml &cacheXml) override;
            
            virtual void addRequests2ProcessingQueue(FRFCFS_Buffer<Message, CoherenceProtocolHandler> &) override;
            virtual void updateCacheLine(void *) override;
        
        public:
            static TypeId GetTypeId(void); // Override TypeId.

            CacheControllerPENDULUM(CacheXml &cacheXml, string &fsm_path,
                        CommunicationInterface* upper_interface, CommunicationInterface* lower_interface,
                        bool cach2Cache, int sharedMemId, CohProtType pType);
            ~CacheControllerPENDULUM();

    };
}

#endif /* _CacheController_H */
