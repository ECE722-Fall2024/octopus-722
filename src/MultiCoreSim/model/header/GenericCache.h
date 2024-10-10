/*
 * File  :      GenericCache.h
 * Author:      Salah Hessien
 * Email :      salahga@mcmaster.ca
 *
 * Created On May 30, 2020
 */

#ifndef _GenericCache_H
#define _GenericCache_H

#include "ns3/ptr.h"
#include "ns3/object.h"
#include "ns3/core-module.h"

namespace ns3
{
  class GenericCacheLine
  {
  public:
    bool valid; // used with associated map only
    bool evicted;
    uint64_t insertCycle;
    uint64_t accessCounter;
    uint64_t accessCycle;
    uint64_t tag;
    uint8_t data[8];
    int state;
    int owner_id;
    bool mru_bit;

    GenericCacheLine()
    {
      tag = 0;
      valid = false;
      evicted = false;
      insertCycle = 0;
      accessCounter = 0;
      accessCycle = 0;
      owner_id = -1;
      for (int i = 0; i < 8; i++)
        data[i] = 0;
    }

    GenericCacheLine(int state, bool valid, uint64_t tag, uint8_t *data, int owner_id = -1) : GenericCacheLine()
    { 
      this->state = state;
      this->valid = valid;
      this->tag = tag;
      this->owner_id = owner_id;

      if (data != NULL)
        memcpy(this->data, data, 8);
    }

    ~GenericCacheLine()
    {
    }

    void copy(GenericCacheLine &x)
    {
      this->valid = x.valid;
      this->evicted = x.evicted;
      this->insertCycle = x.insertCycle;
      this->accessCounter = x.accessCounter;
      this->accessCycle = x.accessCycle;
      this->tag = x.tag;
      this->state = x.state;
      this->owner_id = x.owner_id;
      this->mru_bit = x.mru_bit;

      memcpy(this->data, x.data, sizeof(data));
    }
  };

  // address map format
  class GenericCacheMapFrmt
  {
  public:
    uint32_t idx_set;
    uint64_t tag;
    uint32_t ofst;
    GenericCacheMapFrmt()
    {
      idx_set = 0;
      tag = 0;
      ofst = 0;
    }
    ~GenericCacheMapFrmt()
    {
    }
  };

  enum class ReplcPolicy
  {
    RANDOM = 0,
    LRU,
    MRU,
    LFU,
    MFU,
    FIFO,
    LIFO
  };

  // generic cache template
  class GenericCache : public ns3::Object
  {
  protected:
    GenericCacheLine *m_cache;
    uint16_t m_cacheType;
    uint32_t m_cacheSize;
    uint32_t m_cacheBlkSize;
    uint32_t m_nways;
    uint32_t m_nbanks;
    uint32_t m_bankNumber;
    uint32_t m_nsets;
    int m_initialState;
    Ptr<UniformRandomVariable> uRnd1;
    ReplcPolicy m_replcPolicy;

    void *cache_line_ptr;

    virtual void *getLine(int index)
    {
      cache_line_ptr = (void *)&m_cache[index];
      return cache_line_ptr;
    }

  public:
    struct CacheLineInfo
    {
      bool IsExist;
      bool IsValid;
      bool IsSetFull;
      int state;
      int owner_id;
      int way_idx;
      uint32_t set_idx;
      uint32_t setOfst; // TBDelete
      uint32_t cl_idx;  // TBDelete
    };

    GenericCache(uint32_t size, ReplcPolicy replcPolicy = ReplcPolicy::RANDOM)
    {
      this->m_replcPolicy = replcPolicy;
      m_cache = new GenericCacheLine[size];
      uRnd1 = CreateObject<UniformRandomVariable>();
    }

    GenericCache()
    {
    }

    ~GenericCache()
    {
      delete[] m_cache;
    }

    void SetCacheSize(uint32_t cacheSize)
    {
      m_cacheSize = cacheSize;
    }

    uint32_t GetCacheSize()
    {
      return m_cacheSize;
    }

    void SetCacheBlkSize(uint32_t cacheBlkSize)
    {
      m_cacheBlkSize = cacheBlkSize;
    }

    uint32_t GetCacheBlkSize()
    {
      return m_cacheBlkSize;
    }

    void SetCacheNways(uint32_t nways)
    {
      m_nways = nways;
    }

    uint32_t GetCacheNways()
    {
      return m_nways;
    }
    void SetCacheNBanks(uint32_t nbanks)
    {
      m_nbanks = nbanks;
    }

    uint32_t GetCacheNbanks()
    {
      return m_nbanks;
    }
    void SetCacheBankNum(uint32_t bankNumber)
    {
      m_bankNumber = bankNumber;
    }

    uint32_t GetCacheBankNum()
    {
      return m_bankNumber;
    }

    void SetCacheNsets(uint32_t nsets)
    {
      m_nsets = nsets;
    }

    uint32_t GetCacheNsets()
    {
      return m_nsets;
    }

    void SetCacheType(uint16_t cacheType)
    {
      m_cacheType = cacheType;
    }

    uint16_t GetCacheType()
    {
      return m_cacheType;
    }

    void InitalizeCacheStates(int initialState)
    {
      m_initialState = initialState;
      for (uint32_t i = 0; i < m_nsets * m_nways; i++)
      {
        ((GenericCacheLine*)getLine(i))->state = initialState;
        ((GenericCacheLine*)getLine(i))->valid = false;
        ((GenericCacheLine*)getLine(i))->mru_bit = false;
      }
    }

    void WriteCacheLine(GenericCacheLine line, uint32_t cl_idx)
    {
      *((GenericCacheLine*)getLine(cl_idx)) = line;
    }

    void WriteCacheLine(uint32_t set_idx, uint32_t way_idx, GenericCacheLine line)
    {
      *((GenericCacheLine*)getLine(set_idx * m_nways + way_idx)) = line;
    }

    GenericCacheLine ReadCacheLine(uint32_t cl_idx)
    {
      return *((GenericCacheLine*)getLine(cl_idx));
    }

    GenericCacheLine ReadCacheLine(uint32_t set_idx, uint32_t way_idx)
    {
      return *((GenericCacheLine*)getLine(set_idx * m_nways + way_idx));
    }

    void SetCacheLineState(uint32_t cl_idx, int state)
    {
      ((GenericCacheLine*)getLine(cl_idx))->state = state;
    }

    void SetCacheLineState(uint32_t set_idx, uint32_t way_idx, GenericCacheLine line)
    {
      ((GenericCacheLine*)getLine(set_idx * m_nways + way_idx))->state = line.state;
      ((GenericCacheLine*)getLine(set_idx * m_nways + way_idx))->valid = line.valid;
    }

    int GetCacheLineState(uint32_t cl_idx)
    {
      return ((GenericCacheLine*)getLine(cl_idx))->state;
    }

    int GetCacheLineState(uint64_t phy_addr)
    {
      uint32_t address = GetCacheLineIdx(phy_addr);
      return ((GenericCacheLine*)getLine(address))->state;
    }

    // Map physical address to private cache format
    GenericCacheMapFrmt CpuAddrMap(uint64_t phy_addr)
    {
      /* Map physical address to private cache format
       * PA  is shared memory physical address. For 2MB, PA is 24-bit byte address
       * BO  is the block offset bits = PA & BO_BIT_MASK (i.e. 0000_0000_0000_0000_0011_1111)
       * CI  is cache index bits = (PA >> BO) & CI_BIT_MASK (i.e. 0000_0000_0000_0000_1111_1111)
       * TAG is cache tage bits = PA >> (CI + BO)
       */
      // FixMe: address width in the BMs are higher than 24-bits,
      // so the tag values is not correct. It needs to be trunchated.
      GenericCacheMapFrmt cacheLine;

      cacheLine.idx_set = (phy_addr >> (int)log2(m_cacheBlkSize)) & (m_nsets - 1);
      cacheLine.ofst = phy_addr & (m_cacheBlkSize - 1);
      cacheLine.tag = phy_addr >> ((int)log2(m_cacheBlkSize) + (int)log2(m_nsets));
      return cacheLine;
    }

    uint64_t CpuPhyAddr(GenericCacheMapFrmt cacheLine)
    {
      return (cacheLine.ofst + (cacheLine.idx_set << (int)log2(m_cacheBlkSize)) + (cacheLine.tag << ((int)log2(m_cacheBlkSize) + (int)log2(m_nsets))));
    }

    uint64_t CpuPhyAddr(uint32_t cl_idx)
    {
      GenericCacheLine cacheLine = *((GenericCacheLine*)getLine(cl_idx));
      uint32_t setIdx = cl_idx / m_nways;
      uint32_t ofst = 0;
      return (ofst + (setIdx << (int)log2(m_cacheBlkSize)) + (cacheLine.tag << ((int)log2(m_cacheBlkSize) + (int)log2(m_nsets))));
    }

    uint64_t CpuPhyAddr(uint32_t set_idx, uint32_t way_idx)
    {
      GenericCacheLine cacheLine = ReadCacheLine(set_idx, way_idx);
      uint32_t ofst = 0;
      return (ofst + (set_idx << (int)log2(m_cacheBlkSize)) + (cacheLine.tag << ((int)log2(m_cacheBlkSize) + (int)log2(m_nsets))));
    }

    uint32_t GetCacheLineIdx(uint64_t phy_addr)
    {
      GenericCacheMapFrmt cacheLine;
      cacheLine = CpuAddrMap(phy_addr);
      return cacheLine.idx_set;
    }

    uint32_t GetReplacementLine(uint32_t set_idx)
    {
      uint64_t TempVal;
      uint32_t way2Evict = m_nways;
      GenericCacheLine cacheLine;

      if (m_replcPolicy == ReplcPolicy::RANDOM)
      {
        uRnd1->SetAttribute("Min", DoubleValue(0));
        uRnd1->SetAttribute("Max", DoubleValue(m_nways - 1));
        return uint32_t(uRnd1->GetValue());
      }
      else if (m_replcPolicy == ReplcPolicy::LRU)
      {
        TempVal = -1; // max range

        for (uint32_t wayIdx = 0; wayIdx < m_nways; wayIdx++)
        {
          cacheLine = ReadCacheLine(set_idx, wayIdx);

          if (cacheLine.accessCycle == 0)
          {
            way2Evict = wayIdx;
            break;
          }
          else if (cacheLine.accessCycle < TempVal)
          {
            way2Evict = wayIdx;
            TempVal = cacheLine.accessCycle;
          }
        }

        if (way2Evict == m_nways)
        {
          std::cout << "Replacement LRU hazard detected" << std::endl;
          exit(0);
        }
        return way2Evict;
      }
      else if (m_replcPolicy == ReplcPolicy::MRU)
      {
        TempVal = 0;
        for (uint32_t wayIdx = 0; wayIdx < m_nways; wayIdx++)
        {
          cacheLine = ReadCacheLine(set_idx, wayIdx);

          if (cacheLine.accessCycle >= TempVal)
          {
            way2Evict = wayIdx;
            TempVal = cacheLine.accessCycle;
          }
        }
        return way2Evict;
      }
      else if (m_replcPolicy == ReplcPolicy::LFU)
      {
        TempVal = -1;

        for (uint32_t wayIdx = 0; wayIdx < m_nways; wayIdx++)
        {
          cacheLine = ReadCacheLine(set_idx, wayIdx);

          if (cacheLine.accessCounter == 0)
          {
            way2Evict = wayIdx;
            break;
          }
          else if (cacheLine.accessCounter < TempVal)
          {
            way2Evict = wayIdx;
            TempVal = cacheLine.accessCounter;
          }
        }
        return way2Evict;
      }
      else if (m_replcPolicy == ReplcPolicy::MFU)
      {
        TempVal = 0;
        for (uint32_t wayIdx = 0; wayIdx < m_nways; wayIdx++)
        {
          cacheLine = ReadCacheLine(set_idx, wayIdx);

          if (cacheLine.accessCounter >= TempVal)
          {
            way2Evict = wayIdx;
            TempVal = cacheLine.accessCounter;
          }
        }
        return way2Evict;
      }
      else if (m_replcPolicy == ReplcPolicy::FIFO)
      {
        TempVal = -1;
        for (uint32_t wayIdx = 0; wayIdx < m_nways; wayIdx++)
        {
          cacheLine = ReadCacheLine(set_idx, wayIdx);

          if (cacheLine.insertCycle == 0)
          {
            way2Evict = wayIdx;
            break;
          }
          else if (cacheLine.insertCycle < TempVal)
          {
            way2Evict = wayIdx;
            TempVal = cacheLine.insertCycle;
          }
        }
        return way2Evict;
      }
      else if (m_replcPolicy == ReplcPolicy::LIFO)
      {
        TempVal = 0;
        for (uint32_t wayIdx = 0; wayIdx < m_nways; wayIdx++)
        {
          cacheLine = ReadCacheLine(set_idx, wayIdx);

          if (cacheLine.insertCycle == 0)
          {
            way2Evict = wayIdx;
            break;
          }
          else if (cacheLine.insertCycle >= TempVal)
          {
            way2Evict = wayIdx;
            TempVal = cacheLine.insertCycle;
          }
        }
        return way2Evict;
      }
      else
      {
        std::cout << "UnSupported Replacement Configuration" << std::endl;
        exit(0);
      }
    }

    void ClkClrMRUBits(uint32_t cl_idx)
    {
      uint32_t setOfst = cl_idx * m_nways;
      bool clearFlag = true;
      GenericCacheLine cacheLine;

      for (uint32_t wayIdx = 0; wayIdx < m_nways; wayIdx++)
      {
        cacheLine = ReadCacheLine(setOfst + wayIdx);
        if (cacheLine.mru_bit == 0)
        {
          clearFlag = false;
        }
      }

      if (clearFlag == true)
      {
        for (uint32_t wayIdx = 0; wayIdx < m_nways; wayIdx++)
          ((GenericCacheLine*)getLine(setOfst + wayIdx))->mru_bit = 0;
      }
    }

    int SetMRUBit(uint32_t set_idx, uint32_t way_idx)
    {
      int uWay;
      if (((GenericCacheLine*)getLine(set_idx * m_nways + way_idx))->mru_bit == 1)
      {
        uWay = -1;
      }
      else
      {
        uWay = -2;
      }
      ((GenericCacheLine*)getLine(set_idx * m_nways + way_idx))->mru_bit = 1;

      return uWay;
    }

    int UpdateLRU(uint32_t set_idx, uint32_t way_idx)
    {
      int uWay;
      uWay = SetMRUBit(set_idx, way_idx);
      ClkClrMRUBits(set_idx);
      SetMRUBit(set_idx, way_idx);
      return uWay;
    }

    int GetEmptyCacheLine(uint32_t set_idx)
    {
      GenericCacheLine cacheLine;
      for (uint32_t wayIdx = 0; wayIdx < m_nways; wayIdx++)
      {
        cacheLine = ReadCacheLine(set_idx, wayIdx);
        if (cacheLine.valid == false)
        {
          return wayIdx;
        }
      }
      return -1;
    }

    CacheLineInfo GetCacheLineInfo(uint64_t addr)
    {
      CacheLineInfo lineInfo;
      lineInfo.IsExist = false;
      lineInfo.IsValid = false;
      lineInfo.IsSetFull = true;

      GenericCacheLine cacheLine;

      GenericCacheMapFrmt addrMap = CpuAddrMap(addr);

      uint32_t setOfst = addrMap.idx_set * m_nways;

      lineInfo.way_idx = -1;
      lineInfo.set_idx = addrMap.idx_set;
      lineInfo.state = m_initialState;
      lineInfo.owner_id = -1;

      for (uint32_t wayIdx = 0; wayIdx < m_nways; wayIdx++)
      {
        cacheLine = ReadCacheLine(wayIdx + setOfst); // is it correct? HOSSAM

        // check address tag
        if (cacheLine.valid == true && cacheLine.tag == addrMap.tag && lineInfo.IsValid == false)
        {
          lineInfo.IsExist = true;
          lineInfo.IsValid = cacheLine.valid;
          lineInfo.state = cacheLine.state;
          lineInfo.way_idx = wayIdx;
          lineInfo.owner_id = cacheLine.owner_id;
        }

        // Check if the current Set is full
        if (cacheLine.valid == false)
        {
          lineInfo.IsSetFull = false;
        }
      }

      return lineInfo;
    }

  }; // class GenericCache

  class VictimCache : public ns3::Object
  {
  private:
    GenericCacheLine *m_cache;
    uint32_t m_cacheSize;
    uint32_t m_cacheBlkSize;
    uint32_t m_lines;
    uint32_t m_initialState;
    uint32_t m_wptr;

  public:
    struct CacheLineInfo
    {
      bool IsExist;
      bool IsValid;
      bool IsSetFull;
      int state;
      int line_idx;
    };

    VictimCache(uint32_t size)
    {
      m_cache = new GenericCacheLine[size];
    }

    ~VictimCache()
    {
      delete[] m_cache;
    }

    void SetCacheSize(uint32_t cacheSize)
    {
      m_cacheSize = cacheSize;
    }

    uint32_t GetCacheSize()
    {
      return m_cacheSize;
    }

    void SetCacheBlkSize(uint32_t cacheBlkSize)
    {
      m_cacheBlkSize = cacheBlkSize;
    }

    uint32_t GetCacheBlkSize()
    {
      return m_cacheBlkSize;
    }

    void SetCacheLines(uint32_t lines)
    {
      m_lines = lines;
    }

    uint32_t GetCacheLines()
    {
      return m_lines;
    }

    // Map physical address to private cache format
    GenericCacheMapFrmt CpuAddrMap(uint64_t phy_addr)
    {
      GenericCacheMapFrmt cacheLine;
      cacheLine.idx_set = 0;
      cacheLine.ofst = phy_addr & (m_cacheBlkSize - 1);
      cacheLine.tag = phy_addr >> (int)log2(m_cacheBlkSize);

      return cacheLine;
    }

    uint64_t CpuPhyAddr(uint64_t tag)
    {
      return (tag << ((int)log2(m_cacheBlkSize)));
    }

    void InitalizeCacheStates(int initialState)
    {
      m_initialState = initialState;
      m_wptr = 0;
      for (uint32_t i = 0; i < m_lines; i++)
      {
        m_cache[i].state = initialState;
        m_cache[i].valid = false;
      }
    }

    void WriteNewCacheLine(GenericCacheLine cacheLine)
    {
      m_cache[m_wptr] = cacheLine;
      m_wptr = (m_wptr == m_lines - 1) ? 0 : (m_wptr + 1);
    }

    void WriteCacheLine(uint32_t cl_idx, GenericCacheLine cacheLine)
    {
      m_cache[cl_idx] = cacheLine;
    }

    GenericCacheLine ReadCacheLine(uint32_t cl_idx)
    {
      if (cl_idx > 15)
        std::cout << "victim cache read index = " << cl_idx << std::endl;
      return m_cache[cl_idx];
    }

    GenericCacheLine ReadCacheLine2Evict()
    {
      return m_cache[m_wptr];
    }

    int GetNextWritePtr()
    {
      GenericCacheLine cacheLine;
      int evicted_idx = -1;
      uint64_t prev_cycle = -1;
      for (uint32_t wayIdx = 0; wayIdx < m_lines; wayIdx++)
      {
        cacheLine = ReadCacheLine(wayIdx);
        if (cacheLine.valid == false)
        {
          evicted_idx = wayIdx;
          break;
        }
        else if (cacheLine.evicted)
        {
          if (cacheLine.insertCycle < prev_cycle)
          {
            evicted_idx = wayIdx;
          }
          prev_cycle = cacheLine.insertCycle;
        }
      }
      return evicted_idx;
    }

    CacheLineInfo GetCacheLineInfo(uint64_t addr)
    {
      CacheLineInfo lineInfo;
      lineInfo.IsExist = false;
      lineInfo.IsValid = false;
      lineInfo.IsSetFull = true;

      GenericCacheLine cacheLine;

      GenericCacheMapFrmt addrMap = CpuAddrMap(addr);

      lineInfo.line_idx = -1;
      lineInfo.state = m_initialState;

      for (uint32_t wayIdx = 0; wayIdx < m_lines; wayIdx++)
      {
        cacheLine = ReadCacheLine(wayIdx);

        cacheLine.tag = cacheLine.tag >> (int)log2(m_cacheBlkSize);

        // check address tag
        if (cacheLine.tag == addrMap.tag && lineInfo.IsValid == false)
        {
          lineInfo.IsExist = true;
          lineInfo.IsValid = cacheLine.valid;
          lineInfo.state = cacheLine.state;
          lineInfo.line_idx = wayIdx;
        }

        // Check if the current Set is full
        if (cacheLine.valid == false || cacheLine.evicted == true)
        {
          lineInfo.IsSetFull = false;
        }
      }

      return lineInfo;
    }
  }; // class VictimCache

}

#endif /* _GenericCache_H */
