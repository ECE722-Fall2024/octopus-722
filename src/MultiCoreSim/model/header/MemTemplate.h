/*
 * File  :      MemTemplate.h
 * Author:      Salah Hessien
 * Email :      salahga@mcmaster.ca
 *
 * Created On February 16, 2020
 */

#ifndef _MemTemplate_H
#define _MemTemplate_H

#include "CommunicationInterface.h"

#include "ns3/object.h"
#include <queue>
#include <string>

namespace ns3
{
  // Generic FIFO Interface
  template <typename T>
  class GenericFIFO : public ns3::Object
  {
  private:
    std::queue<T> m_FIFO;
    uint16_t m_fifoDepth;

  public:
    void SetFifoDepth(int fifoDepth)
    {
      m_fifoDepth = fifoDepth;
    }

    int GetFifoDepth()
    {
      return m_fifoDepth;
    }

    void InsertElement(T msg)
    {
      m_FIFO.push(msg);
    }

    void PopElement()
    {
      m_FIFO.pop();
    }

    T GetFrontElement()
    {
      return m_FIFO.front();
    }

    void UpdateFrontElement(T msg)
    {
      m_FIFO.front() = msg;
    }

    int GetQueueSize()
    {
      return m_FIFO.size();
    }

    bool IsEmpty()
    {
      return m_FIFO.empty();
    }

    bool IsFull()
    {
      return (m_FIFO.size() == m_fifoDepth) ? true : false;
    }
  };

  // CPU FIFO Interface
  class CpuFIFO : public CommunicationInterface
  {
  public:
    enum REQTYPE
    {
      READ = 0,
      WRITE = 1,
      REPLACE = 2
    };
    // A request  contains information on
    // its own memory request, type, and cycle.
    struct ReqMsg
    {
      uint64_t msgId;
      uint16_t reqCoreId;
      uint64_t addr;
      uint64_t cycle;
      uint64_t fifoInserionCycle;
      REQTYPE type;
      uint8_t data[8];
    };

    struct RespMsg
    {
      uint64_t msgId;
      uint64_t addr;
      uint64_t reqcycle;
      uint64_t cycle;
    };

  public:
    GenericFIFO<ReqMsg> m_txFIFO;
    GenericFIFO<RespMsg> m_rxFIFO;

    CpuFIFO(int id, int FIFOs_depth) : CommunicationInterface(id)
    {
      m_txFIFO.SetFifoDepth(FIFOs_depth);
    }

    virtual bool peekMessage(Message *out_msg)
    {
      if (!m_txFIFO.IsEmpty())
      {
        *out_msg = Message(m_txFIFO.GetFrontElement().msgId,          //id
                           m_txFIFO.GetFrontElement().addr,           //Addr
                           m_txFIFO.GetFrontElement().cycle,          //Cycle
                           (uint64_t)m_txFIFO.GetFrontElement().type, //Complementary_value
                           m_txFIFO.GetFrontElement().reqCoreId);     //Owner
        return true;
      }
      else
        return false;
    }

    virtual void popFrontMessage()
    {
      m_txFIFO.PopElement();
    }

    virtual bool pushMessage(Message &msg, uint64_t cycle, MessageType type = MessageType::REQUEST)
    {
      RespMsg resp_msg = {.msgId = msg.msg_id,
                          .addr = msg.addr,
                          .reqcycle = msg.cycle,
                          .cycle = cycle};
      m_rxFIFO.InsertElement(resp_msg);

      return true;
    }
  };
}

#endif /* _MemTemplate */
