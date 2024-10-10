/*
 * File  :      CacheControllerPENDULUM.cpp
 * Author:      Safin Bayes
 * Email :      bayess@mcmaster.ca
 *
 * Created On March 09, 2022
 */

#include "../header/CacheControllerPENDULUM.h"
#include "../header/TimerCache.h";
#include "../header/LLCTimerCache.h";

namespace ns3
{
    // override ns3 type
    TypeId CacheControllerPENDULUM::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::CacheControllerPENDULUM").SetParent<Object>();
        return tid;
    }

    // private controller constructor
    CacheControllerPENDULUM::CacheControllerPENDULUM(CacheXml &cacheXml, string &fsm_path, CommunicationInterface *upper_interface,
                                     CommunicationInterface *lower_interface, bool cach2Cache,
                                     int sharedMemId, CohProtType pType) : CacheController(cacheXml, fsm_path, upper_interface,
                                     lower_interface, cach2Cache, sharedMemId, pType)
    {
        timer_val = 100; //TODO: Will receive from the XML file
        // Pointer to TimerCacheFrmt?
        num_cacheLines = cacheXml.GetCacheSize() / cacheXml.GetBlockSize();
    }

    CacheControllerPENDULUM::~CacheControllerPENDULUM()
    {}

    void CacheControllerPENDULUM::initializeCacheHandler(CacheXml &cacheXml)
    {
        uint32_t cacheLines = cacheXml.GetCacheSize() / cacheXml.GetBlockSize();
        m_cache = new TimerCache(cacheLines);
        m_cache->SetCacheType(cacheXml.GetMappingType());
        m_cache->SetCacheSize(cacheXml.GetCacheSize());
        m_cache->SetCacheBlkSize(cacheXml.GetBlockSize());
        m_cache->SetCacheNways(cacheXml.GetNWays());
        m_cache->SetCacheNsets(cacheLines / cacheXml.GetNWays());
    }

    void CacheControllerPENDULUM::addRequests2ProcessingQueue(FRFCFS_Buffer<Message, CoherenceProtocolHandler> &buf)
    {

        if (!self_msgs.empty())
        {
            Message msg = self_msgs[0];

            if (buf.pushFront(msg))
                self_msgs.erase(self_msgs.begin());
        }

        // Add the function here to check the timeout of every cacheline 
        vector <Message> timeout_requests = ((TimerCache*)m_cache)->checktimeout(this->m_cache_cycle, num_cacheLines, this->m_core_id);

        for (Message msg : timeout_requests)
        {
            buf.pushFront(msg);
            delete &msg;
        }

        CacheController::addRequests2ProcessingQueue(buf);
    }

    void CacheControllerPENDULUM::callActionFunction(ControllerAction action)
    {
        switch(action.type)
        {
            case ControllerAction::Type::TIMER_ACTION: this->timer_actions(action.data); return;
            case ControllerAction::Type::ADD_SHARER: this->add_sharer(action.data); return;
            case ControllerAction::Type::REMOVE_SHARER: this->remove_sharer(action.data); return;

            default: CacheController::callActionFunction(action); return;
        }
    }

    void CacheControllerPENDULUM::timer_actions(void *data_ptr)
    {
        Message *msg = (Message *)data_ptr;

        TimerCache::CacheLineInfo cache_line_info = this->m_cache->GetCacheLineInfo(msg->addr);
        uint32_t set_index = cache_line_info.set_idx;
        uint32_t way_index = cache_line_info.way_idx;

        TimerCacheFrmt *cacheline = (TimerCacheFrmt *)data_ptr;
        int new_timeout = (msg->complementary_value == 1) ? 0 : this->m_cache_cycle + timer_val;      // TODO Pass it in the data array 
        cacheline->timeout = new_timeout;

        this->m_cache->WriteCacheLine(set_index, way_index, *cacheline);

        delete[](data_ptr);
    }

    void CacheControllerPENDULUM::add_sharer(void *data_ptr)
    {
        Message *msg = (Message *)data_ptr;

        LLCTimerCache::CacheLineInfo cache_line_info = m_cache->GetCacheLineInfo(msg->addr);
        uint32_t set_index = cache_line_info.set_idx;
        uint32_t way_index = cache_line_info.way_idx;

        LLCTimerCacheFrmt *cacheline = (LLCTimerCacheFrmt *)data_ptr;
        cacheline->sharer_count++;
        this->m_cache->WriteCacheLine(set_index, way_index, *cacheline);

        delete[](data_ptr);
    }

    void CacheControllerPENDULUM::remove_sharer(void *data_ptr)
    {
        Message *msg = (Message *)data_ptr;
        
        // Get cache info from data_ptr
        LLCTimerCache::CacheLineInfo cache_line_info = m_cache->GetCacheLineInfo(msg->addr);
        uint32_t set_index = cache_line_info.set_idx;
        uint32_t way_index = cache_line_info.way_idx;

        LLCTimerCacheFrmt *cacheline = (LLCTimerCacheFrmt *)data_ptr;

        // Read cacheline
        if(cacheline->sharer_count == 1)
        {
            // Trigger Sharers_0 event
            msg->source = Message::Source::SELF;
            self_msgs.push_back(*msg);         
        }

        //Write cachline
        cacheline->sharer_count--;
        this->m_cache->WriteCacheLine(set_index, way_index, *cacheline);

        delete[](data_ptr);
    }

}
