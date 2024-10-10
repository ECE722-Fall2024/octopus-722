/*
 * File  :      CoherenceProtocolHandler.h
 * Author:      Mohammed Ismail
 * Email :      ismaim22@mcmaster.ca
 *
 * Created On June 23, 2021
 */

#ifndef _CoherenceProtocolHandler_H
#define _CoherenceProtocolHandler_H

#include "ns3/FSMReader.h"
#include "ns3/FRFCFS_Buffer.h"
#include "ns3/Logger.h"
#include "ns3/IdGenerator.h"
#include "ns3/CacheDataHandler.h"
#include "ns3/SNOOPProtocolCommon.h"

#include <string.h>

namespace ns3
{
    class CoherenceProtocolHandler
    {
    protected:
        int m_core_id;
        int m_shared_memory_id;
        bool m_cache2Cache;
        int m_reqWbRatio;
        uint32_t m_cycle;

        FSMReader *m_fsm;
        CacheDataHandler *m_data_handler;

    public:
        CoherenceProtocolHandler(CacheDataHandler *cache, const std::string& fsm_path, int coreId, int sharedMemId);
        virtual ~CoherenceProtocolHandler();

        virtual const std::vector<ControllerAction>& processRequest(Message& request_msg) = 0;
        virtual FRFCFS_State getRequestState(const Message &, FRFCFS_State) = 0;
        virtual void initializeCacheStates();
        virtual void createDefaultCacheLine(uint64_t address, GenericCacheLine *cache_line) {};
        inline FSMReader * fsm() { return m_fsm; }

        virtual void updateCycle(uint64_t cycle);
    };
}

#endif
