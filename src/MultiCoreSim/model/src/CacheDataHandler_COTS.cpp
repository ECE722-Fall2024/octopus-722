/*
 * File  :      CacheDataHandler_COTS.cpp
 * Author:      Mohammed Ismail
 * Email :      ismaim22@mcmaster.ca
 *
 * Created On May 20, 2022
 */
#include "../header/CacheDataHandler_COTS.h"
#include "../header/Protocols/Protocols.h"
#include "alloc_setting.h"
//#define DEBUG_MSGS
//#define DONT_EVICT_TRANSIENT_LINES
namespace ns3
{
    CacheDataHandler_COTS::CacheDataHandler_COTS(CacheXml &cacheXml, ReplacementPolicy* policy)
        : CacheDataHandler(cacheXml, policy)
    {
        line_added2PWB = false;
        address_of_recently_added2PWB = 0;
    }

    CacheDataHandler_COTS::~CacheDataHandler_COTS()
    {
    }

    inline void *CacheDataHandler_COTS::getLine(uint64_t set, int way)
    {
        if (way >= 0)
            return CacheDataHandler::getLine(set, way);
        else if (checkMSHR(set))
            return (void *)&m_miss_status_holding_regs[set].second;
        else if (checkPWB(set))
            return (void *)&m_pending_write_back_regs[set];

        return NULL;
    }

    bool CacheDataHandler_COTS::findline(uint64_t address, uint64_t *set, int *way)
    {
        if (CacheDataHandler::findline(address, set, way))
            return true;

        *way = -1;

        if (checkMSHR(mask_offset(address)))
        {
            *set = mask_offset(address);
            return true;
        }
        else if (checkPWB(mask_offset(address)))
        {
            *set = mask_offset(address);
            return true;
        }

        return false;
    }

    void CacheDataHandler_COTS::writeLine2MSHR(uint64_t address, GenericCacheLine *line)
    {
        if (line->valid == false)
            return;
        assert(m_miss_status_holding_regs.size() < MSHR_max_size);
        // write cycle count to MSHR entry
        MSHRMetadata metadata = { m_cycle, 0, 0 };
        std::pair<MSHRMetadata, GenericCacheLine> new_line =
            std::make_pair(metadata, *line);
        m_miss_status_holding_regs[mask_offset(address)] = new_line;
        m_miss_status_holding_regs[mask_offset(address)].second.m_block_size =
            this->m_block_size;
    }

    void CacheDataHandler_COTS::moveLine2WB(uint64_t set, int way)
    {
        // crash if trying to write to full WB buffer
        assert(m_pending_write_back_regs.size() < WB_max_size);
        GenericCacheLine *line = (GenericCacheLine *)getLine(set, way);
        m_pending_write_back_regs[calculate_address(line->tag, set)] = *line;
        line->valid = false;

        address_of_recently_added2PWB = calculate_address(line->tag, set);
        line_added2PWB = true;
    }

    bool CacheDataHandler_COTS::updateLineData(uint64_t address, const uint8_t *data,
        CoherenceProtocolHandler *m_protocol, uint64_t msg_id, int core_id)
    {
        uint64_t set;
        int way;

        if (CacheDataHandler::findline(address, &set, &way))
        {
            #ifdef DEBUG_MSGS
            std::cout << "Addr: " << address << " updating set "
                << set << ", way " << way << std::endl;
            #endif
            bool ret = CacheDataHandler::updateLineData(address, data,m_protocol,
                msg_id, core_id);
            #ifdef ALLOC_ON_MISS
            if (checkMSHR(mask_offset(address))) {
                m_miss_status_holding_regs.erase(mask_offset(address));
            }
            #endif
            std::cerr << msg_id << "," << address << "," \
                << "wrCache" << "," <<  core_id << ","<< m_cycle << "\n";
            //if (core_id == 10) {
                std::cerr << msg_id << "," << address << "," \
                    << "termina" << "," << core_id 
                    << ","<< m_cycle << "\n";
            //}

            return ret;
        }

        if (checkMSHR(mask_offset(address)))
        {
            #ifdef ALLOC_ON_MISS
            std::cout<<"This should not happen!\n";
            #endif
            m_miss_status_holding_regs[mask_offset(address)].second.copyData(data);
            int victim = findEmptyWay(address);
            if (findEmptyWay(address) == -1) {
                victim = chooseEvictionWay(set);
                // TODO: check if victim line is dirty
                if (!isLineDirty(set, victim, m_protocol))
                {
                    // if clean, silently evict
                    ((GenericCacheLine*)getLine(set, victim))->valid = false;
                }
                else
                {
                    // if WB has a space, send victim to that space.
                    if (m_pending_write_back_regs.size() < WB_max_size)
                    {
                        moveLine2WB(set, victim);
                    }
                    // if not, let the pending line sit in the MSHR;
                    // do not write the line to cache
                    else
                    {
                        std::cout << address<<": WB full; keeping in MSHR\n";
                        m_miss_status_holding_regs[mask_offset(address)].first.msg_id
                            = msg_id;
                        m_miss_status_holding_regs[mask_offset(address)].first.core_id
                            = core_id;
                        return true;  //TODO: return true or false?
                    }
                }
                
            }
            //std::cout << "wb size: " << m_pending_write_back_regs.size() << "\n";
            #ifdef DEBUG_MSGS
            std::cout << "Addr: "<< address
                 << " allocating and writing set " << set << ", way "
                 << victim << " on refill\n";
            #endif
            if (writeCacheLine(address, &m_miss_status_holding_regs[mask_offset(address)].second))
            {
                std::cerr << msg_id << "," << address << "," \
                << "wrCache" << "," <<  core_id << ","<< m_cycle << "\n";
                //if (core_id == 10) {
                    std::cerr << msg_id << "," << address << "," \
                        << "termina" << "," << core_id 
                        << ","<< m_cycle << "\n";
                //}

                m_miss_status_holding_regs.erase(mask_offset(address));
                return true;
            }
            return false;
        }
        else if (checkPWB(mask_offset(address)))
        {
            m_pending_write_back_regs[mask_offset(address)].copyData(data);
            std::cerr << msg_id << "," << address << "," \
                << "wrCache" << "," <<  core_id << ","<< m_cycle << "\n";
            //if (core_id == 10) {
                std::cerr << msg_id << "," << address << "," \
                    << "termina" << "," << core_id 
                    << ","<< m_cycle << "\n";
            //}
            return true;
        }

        return false;
    }

    bool CacheDataHandler_COTS::freeUpSpace(const Message &msg,
        CoherenceProtocolHandler *m_protocol)
    {
        // we don't need to start allocation if alloc-on-refill
        #ifndef ALLOC_ON_MISS
        return false;
        #endif

        uint64_t set;
        int way;
        if (findline(msg.addr, &set, &way)) 
        {
            std::cout << "error in freeUpSpace()\n";
            exit(0);
        }
        int victim = chooseEvictionWay(set);
        // Check if victim is dirty
        if (!isLineDirty(set, victim, m_protocol))
        {
            // if clean, silently evict
            GenericCacheLine *line =
                (GenericCacheLine *)getLine(set, victim);
            line->valid = false;
        }
        else
        {
            // If we don't evict transient lines, return false if
            // the line is transient
            #ifdef DONT_EVICT_TRANSIENT_LINES
            GenericCacheLine *line = 
                (GenericCacheLine *)getLine(set, victim);
            if (m_protocol->fsm()->isStall(line->state, 
                msg.complementary_value)) 
            {
                return false;
            }
            #endif
            // check that WB isn't full; if so, return false
            assert(m_pending_write_back_regs.size() <= WB_max_size);
            if (m_pending_write_back_regs.size() == WB_max_size) return false;
            // move to WB
            moveLine2WB(set, victim);
        }
        // ensure that preallocated lines aren't evicted prematurely
        m_replacement_policy->update(set, victim, UINT64_MAX);
        return true;
    }

    // check if (alloc-on-miss) empty way or (alloc-on-refill) space in MSHR
    // We don't check WB size here; that happens upon refill
    bool CacheDataHandler_COTS::findSpace(uint64_t address)
    {
        #ifdef ALLOC_ON_MISS
        return (findEmptyWay(address) == -1) ? false : true;
        #else
        assert(m_miss_status_holding_regs.size() <= MSHR_max_size);
        return (m_miss_status_holding_regs.size() == MSHR_max_size) ?
            false : true;
        #endif
    }
        
    bool CacheDataHandler_COTS::updateLineBits(uint64_t address, GenericCacheLine *line)
    {
        bool return_value = CacheDataHandler::updateLineBits(address, line);

        uint64_t set;
        int way;
        if (findline(address, &set, &way))
        {
            if (way < 0 && line->valid == false)
            {
                if (checkMSHR(set))
                    m_miss_status_holding_regs.erase(set);
                else if (checkPWB(set))
                    m_pending_write_back_regs.erase(set);
            }
        }
        else if (line->valid)
        {
            #ifdef ALLOC_ON_MISS
            way = findEmptyWay(address);
            assert(way != -1);
            #ifdef DEBUG_MSGS
            std::cout << "Addr: "<< address  
                << " reserving set " << set << ", way "
                << way << " on miss\n";
            #endif
            writeCacheLine_bypassLatency(address, line);
            // how can I tell if a cache line has been allocated on miss
            // but not had its data arrive yet? Is it valid=true but data=NULL?

            // ensure that preallocated lines aren't evicted prematurely
            m_replacement_policy->update(set, way, UINT64_MAX);
            #else
            writeLine2MSHR(address, line);
            #endif
        }
        return return_value;
    }

    void CacheDataHandler_COTS::updateCycle(uint64_t cycle)
    {
        CacheDataHandler::updateCycle(cycle);
        // if alloc-on-refill, check if pending lines in MSHR
        #ifndef ALLOC_ON_MISS
        for (;;)
        {
            // get pending entry and evict if possible 
            uint64_t addr;
            if (!getPendingMSHREntry(&addr)) break;
            if (m_pending_write_back_regs.size() == WB_max_size) break;
            uint64_t set;
            int way;
            findline(addr, &set, &way);
            set = calculate_set(addr);
            // the address of the pending line should not be in cache 
            assert(way == -1); 
            // get eviction way based on MSHR entry's corresponding set
            way = chooseEvictionWay(set);
            // TODO: Should we check if the line is clean, and silently evict if so?
            // evict victim; there should be space in the WB now
            moveLine2WB(set, way);
            // send MSHR entry to cache
            assert(writeCacheLine(addr, &m_miss_status_holding_regs[addr].second));
            MSHRMetadata metadata = m_miss_status_holding_regs[addr].first;
            std::cerr << metadata.msg_id << "," << addr << "," \
            << "wrCache" << "," <<  metadata.core_id << ","<< m_cycle << "\n";
            //if (metadata.core_id == 10) {
                std::cerr << metadata.msg_id << "," << addr << "," \
                    << "termina" << "," << metadata.core_id 
                    << ","<< m_cycle << "\n";
            //}

            // remove MSHR entry
            m_miss_status_holding_regs.erase(mask_offset(addr));
        }
        #endif
    }

    bool CacheDataHandler_COTS::getPendingMSHREntry(uint64_t *addr)
    {
        //GenericCacheLine entry;
        uint64_t lowest_cycle = UINT64_MAX;
        uint64_t address = 0;
        bool any_pending = false;
        // loop through MSHR, searching for entry with a full data section
        // and the lowest cycle count (to return the oldest request)
        for (
            auto it = m_miss_status_holding_regs.begin();
            it != m_miss_status_holding_regs.end();
            ++it
        )
        {
            // if entry's data is empty, line is not pending
            if (it->second.second.m_data == NULL) continue;
            any_pending = true;
            // if entry's cycle number is lower, it might be first in FCFS
            if (it->second.first.timestamp < lowest_cycle) 
            {
                // copy the entry to our variable
                //entry = it->second.second;
                lowest_cycle = it->second.first.timestamp;
                address = it->first;
            }
        }
        *addr = address;
        return any_pending;
    }

    int CacheDataHandler_COTS::chooseEvictionWay(uint64_t set)
    {
        int way;
        
        m_replacement_policy->getReplacementCandidate(set, &way);
        return way;
    }

    // Check if a line has recently been added to WB; set "address" to its address
    // Clear the flag showing that the line has been recently added
    // based on clear_flag
    bool CacheDataHandler_COTS::addressOfLinePendingWB(bool clear_flag, uint64_t *address)
    {
        if (line_added2PWB)
        {
            *address = address_of_recently_added2PWB;
            line_added2PWB = clear_flag ? false : line_added2PWB;
            return true;
        }

        return false;
    }

    bool CacheDataHandler_COTS::isReady(uint64_t address)
    {
        if (checkMSHR(mask_offset(address)) || checkPWB(mask_offset(address)))
            return true;
        return CacheDataHandler::isReady(address);
    }

    // This function is very hacky
    // Because the cache does not expose a dirty bit, we check
    // if a line is dirty by directly querying the coherence protocol
    // This completely violates information hiding, so we have to 
    // redefine the EventId enum and add some hacky functions to expose
    // parts of other classes that normally shouldn't be.
    bool CacheDataHandler_COTS::isLineDirty(uint64_t set, uint64_t way, 
        CoherenceProtocolHandler *m_protocol)
    {
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
            Other_PutM,

            OwnData,
            OwnData_Execlusive,
        };

        int next_state;
        vector<int> actions;
        GenericCacheLine *cache_line = (GenericCacheLine*)getLine(set,way);

//        CacheDataHandler::readLineBits(address, &cache_line);
        m_protocol->fsm()->getTransition(
            cache_line->state,
            static_cast<int>(EventId::Replacement),
            next_state,
            actions
        );  
//        std::cout << "current state: " << cache_line->state
//            << ", next state: " << next_state << "\n";
//        std::cout << actions.size() << std::endl;
        return (actions.size() == 0) ? false : true;
    }

}
