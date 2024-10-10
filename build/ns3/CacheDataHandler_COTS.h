/*
 * File  :      CacheDataHandler_COTS.h
 * Author:      Mohammed Ismail
 * Email :      ismaim22@mcmaster.ca
 *
 * Created On May 20, 2022
 */

#ifndef _CacheDataHandler_COTS_H
#define _CacheDataHandler_COTS_H

#include "CacheDataHandler.h"

#include <map>

namespace ns3
{
    class CoherenceProtocolHandler;

    struct MSHRMetadata {
        uint64_t timestamp;
        uint64_t msg_id;
        int core_id;
    };

    class CacheDataHandler_COTS : public CacheDataHandler
    {
    protected:
        std::map<uint64_t, std::pair<MSHRMetadata, GenericCacheLine>>
            m_miss_status_holding_regs; //MSHR
        std::map<uint64_t, GenericCacheLine> m_pending_write_back_regs; //PWB
        uint64_t MSHR_max_size = 10;
        uint64_t WB_max_size = 10;

        bool line_added2PWB;
        uint64_t address_of_recently_added2PWB;

        virtual inline void * getLine(uint64_t set, int way) override;
        virtual bool findline(uint64_t address, uint64_t *set, int *way) override;

        int chooseEvictionWay(uint64_t set);
        void moveLine2WB(uint64_t set, int way);

        bool getPendingMSHREntry(uint64_t *addr);

        inline bool checkMSHR(uint64_t address)
        {
            return m_miss_status_holding_regs.find(address) !=
                        m_miss_status_holding_regs.end();
        }

        inline bool checkPWB(uint64_t address)
        {
            return m_pending_write_back_regs.find(address) !=
                        m_pending_write_back_regs.end();
        }

        inline uint64_t mask_offset(uint64_t address)
        {
            return address & ~((uint64_t)m_block_size - 1);
        }

    public:
        CacheDataHandler_COTS(CacheXml &cacheXml, ReplacementPolicy* policy);
        virtual ~CacheDataHandler_COTS();

        virtual void writeLine2MSHR(uint64_t address, GenericCacheLine *line);

        virtual bool updateLineData(uint64_t address, const uint8_t *data,
            CoherenceProtocolHandler *m_protocol, uint64_t msg_id, int core_id) override;
        virtual bool updateLineBits(uint64_t address, GenericCacheLine *line) override;

        virtual bool isReady(uint64_t address) override;
        virtual bool freeUpSpace(const Message &msg, 
            CoherenceProtocolHandler *m_protocol) override;
        virtual bool findSpace(uint64_t address) override;
        bool addressOfLinePendingWB(bool clear_flag, uint64_t *address);
        virtual void updateCycle(uint64_t cycle) override;
        bool isLineDirty(uint64_t set, uint64_t way, CoherenceProtocolHandler *m_protocol);
    };
}

#endif /* _CacheDataHandler_COTS_H */
