/*
 * File  :      CacheDataHandler.h
 * Author:      Mohammed Ismail
 * Email :      ismaim22@mcmaster.ca
 *
 * Created On April 27, 2022
 */

#ifndef _CacheDataHandler_H
#define _CacheDataHandler_H

#include "CacheXml.h"
#include "GenericCacheLine.h"
#include "ns3/core-module.h"

#include "ns3/ReplacementPolicy.h"
#include "CommunicationInterface.h"

#include <math.h>

namespace ns3
{
    class CoherenceProtocolHandler;
    // enum class ReplcPolicy
    // {
    //     RANDOM = 0,
    //     LRU,
    //     MRU,
    //     LFU,
    //     MFU,
    //     FIFO,
    //     LIFO
    // };

    class CacheDataHandler
    {
    protected:
        GenericCacheLine *m_cache;
        uint32_t m_block_size;
        uint32_t m_ways_count;
        uint32_t m_sets_count;

        // ReplcPolicy m_replacement_policy;
        ReplacementPolicy *m_replacement_policy;

        uint32_t m_cycle;
        uint32_t m_data_access_latency;
        uint32_t m_ready_cycle;

        virtual inline void *getLine(uint64_t set, int way)
        {
            return (void *)&m_cache[set * m_ways_count + way];
        }

        inline uint64_t calculate_tag(uint64_t address)
        {
            return (address >> ((int)log2(m_block_size) + (int)log2(m_sets_count)));
        }

        inline uint64_t calculate_set(uint64_t address)
        {
            return ((address >> (int)log2(m_block_size)) & (m_sets_count - 1));
        }

        inline uint64_t calculate_address(uint64_t tag, uint64_t set)
        {
            return (tag << ((int)log2(m_block_size) + (int)log2(m_sets_count))) |
                   (set << ((int)log2(m_block_size)));
        }

        virtual bool findline(uint64_t address, uint64_t *set, int *way);

    public:
        CacheDataHandler(CacheXml &cacheXml, ReplacementPolicy* policy);
        virtual ~CacheDataHandler();

        virtual void initializeCacheStates(int initialState);
        virtual void initializeCacheLine(GenericCacheLine *line);

        virtual bool writeCacheLine_bypassLatency(uint64_t address, GenericCacheLine *line);
        virtual bool writeCacheLine(uint64_t address, GenericCacheLine *line);
        virtual bool updateLineBits(uint64_t address, GenericCacheLine *line);
        virtual bool updateLineData(uint64_t address, const uint8_t *data,
            CoherenceProtocolHandler *m_protocol, uint64_t msg_id, int core_id);

        virtual bool readCacheLine(uint64_t address, GenericCacheLine *out_line = NULL);
        virtual bool readLineBits(uint64_t address, GenericCacheLine *out_line = NULL);
        virtual bool freeUpSpace(const Message &msg,
            CoherenceProtocolHandler *m_protocol) = 0;
        virtual bool findSpace(uint64_t address) = 0;

        int findEmptyWay(uint64_t address);

        uint64_t getEvictionCandidate(uint64_t address, GenericCacheLine *line);
        
        virtual void updateCycle(uint64_t cycle);
        virtual bool isReady();
        virtual bool isReady(uint64_t address);
    };
}

#endif /* _CacheDataHandler_H */
