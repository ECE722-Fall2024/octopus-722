/*
 * File  :      BusArbiter.cc
 * Author:      Salah Hessien
 * Email :      salahga@mcmaster.ca
 *
 * Created On February 21, 2020
 */

#include "../header/BusArbiter.h"

namespace ns3
{

  // override ns3 type
  TypeId BusArbiter::GetTypeId(void)
  {
    static TypeId tid = TypeId("ns3::BusArbiter")
                            .SetParent<Object>();
    return tid;
  }

  BusArbiter::BusArbiter(std::list<BusIfFIFO *> associatedPrivCacheBusIfFIFO,
                         BusIfFIFO *assoicateLLCBusIfFIFO,
                         ns3::Ptr<ns3::InterConnectFIFO> interConnectFIFO)
  {
    // default
    m_dt = (1.0);
    m_clkSkew = 0;
    m_cpuCore = 4;
    m_arbiCycle = 0;
    m_reqclks = 4;
    m_respclks = 50;
    m_workconserv = false;
    m_reqCoreCnt = 0;
    m_respCoreCnt = 0;
    m_logFileGenEnable = 0;
    m_cacheBlkSize = 64;
    m_sharedCacheBusIfFIFO = assoicateLLCBusIfFIFO;
    m_busIfFIFO = associatedPrivCacheBusIfFIFO;
    m_interConnectFIFO = interConnectFIFO;
    m_bus_arb = BusARBType::PISCOT_ARB;
    m_cohProType = CohProtType::SNOOP_PMSI;
    m_bus_arch = "split";
    m_bus_arbiter = "PISCOT";
    m_reqbus_arb = "TDM";
    m_respbus_arb = "FCFS";
    m_maxPendingReq = 1;
    m_PndReq = false;
    m_PndResp = false;
    m_cach2Cache = false;
    m_TimeOut = 0;
    m_FcFsPndMemResp = false;
    m_PndPutMChk = false;
    m_DirectTransfer = false;
    m_IdleSlot = false;
    m_PndMemResp = false;
    m_PndWB = false;
    for (int i = 0; i < 32; i++)
    {
      m_ReqWbFlag[i] = true;
    }
    m_stallDetectionEnable = true;
    m_stall_cnt = 0;
  }

  BusArbiter::~BusArbiter()
  {
  }

  void BusArbiter::SetCacheBlkSize(uint32_t cacheBlkSize)
  {
    m_cacheBlkSize = cacheBlkSize;
  }

  void BusArbiter::SetDt(double dt)
  {
    m_dt = dt;
  }

  int BusArbiter::GetDt()
  {
    return m_dt;
  }

  void BusArbiter::SetClkSkew(double clkSkew)
  {
    m_clkSkew = clkSkew;
  }

  void BusArbiter::SetIsWorkConserv(bool workConservFlag)
  {
    m_workconserv = workConservFlag;
  }

  void BusArbiter::SetNumPrivCore(int nPrivCores)
  {
    m_cpuCore = nPrivCores;
  }

  void BusArbiter::SetNumReqCycles(int ncycle)
  {
    m_reqclks = ncycle;
  }

  void BusArbiter::SetNumRespCycles(int ncycle)
  {
    m_respclks = ncycle;
  }

  void BusArbiter::SetCache2Cache(bool cach2Cache)
  {
    m_cach2Cache = cach2Cache;
  }

  void BusArbiter::SetBusArchitecture(string bus_arch)
  {
    m_bus_arch = bus_arch;
  }

  void BusArbiter::SetBusArbitration(string bus_arb)
  {
    m_bus_arbiter = bus_arb;
  }

  void BusArbiter::SetReqBusArb(string reqbus_arb)
  {
    m_reqbus_arb = reqbus_arb;
  }

  void BusArbiter::SetRespBusArb(string respbus_arb)
  {
    m_respbus_arb = respbus_arb;
  }

  void BusArbiter::SetCohProtType(CohProtType ptype)
  {
    m_cohProType = ptype;
  }

  void BusArbiter::SetMaxPendingReq(int maxPendingReq)
  {
    m_maxPendingReq = maxPendingReq;
  }

  void BusArbiter::SetLogFileGenEnable(bool logFileGenEnable)
  {
    m_logFileGenEnable = logFileGenEnable;
  }

  void BusArbiter::SendMemCohrMsg(BusIfFIFO::BusReqMsg msg, bool BroadCast = false)
  {
    for (std::list<BusIfFIFO *>::iterator it2 = m_busIfFIFO.begin(); it2 != m_busIfFIFO.end(); it2++)
    {
      if (msg.reqCoreId == (*it2)->m_interface_id || BroadCast == true)
      {
        if (!(*it2)->m_rxMsgFIFO.IsFull())
        {
          (*it2)->m_rxMsgFIFO.InsertElement(msg);
        }
        else
        {
          if (m_logFileGenEnable)
          {
            std::cout << "BusArbiter cannot insert new messages into the buffers" << std::endl;
          }
          exit(0);
        }
      }
    }
  }

  void BusArbiter::SendData(BusIfFIFO::BusRespMsg msg, AGENT agent)
  {
    // send data to core
    if (agent == AGENT::CORE)
    {

      Logger::getLogger()->updateRequest(msg.msgId, Logger::EntryId::RESP_BUS_CHECKPOINT);

      for (std::list<BusIfFIFO *>::iterator it2 = m_busIfFIFO.begin(); it2 != m_busIfFIFO.end(); it2++)
      {
        if (msg.reqCoreId == (*it2)->m_interface_id)
        {
          if (!(*it2)->m_rxRespFIFO.IsFull())
          {
            if (m_logFileGenEnable)
            {
              std::cout << "\nBusArbiter: Cpu/Mem Id " << msg.respCoreId << " Sent Data to Core " << msg.reqCoreId << " ============================================== " << m_arbiCycle << "\n\n";
            }
            (*it2)->m_rxRespFIFO.InsertElement(msg);
          }
          else
          {
            if (m_logFileGenEnable)
            {
              std::cout << "BusArbiter cannot insert new messages into the buffers" << std::endl;
            }
            exit(0);
          }
        }
      }
    }

    if (agent == AGENT::SHAREDMEM)
    {
      if(msg.msgId == 0)
        Logger::getLogger()->registerReplacementMessage(msg.respCoreId);

      msg.reqCoreId = m_sharedCacheBusIfFIFO->m_interface_id;
      if (!m_sharedCacheBusIfFIFO->m_rxRespFIFO.IsFull())
      {
        m_sharedCacheBusIfFIFO->m_rxRespFIFO.InsertElement(msg);
      }
      else
      {
        if (m_logFileGenEnable)
        {
          std::cout << "BusArbiter cannot insert new messages into LLC RxResp buffers" << std::endl;
        }
        exit(0);
      }
    }

    if (agent == AGENT::INTERCONNECT)
    {
      if (!m_interConnectFIFO->m_RespMsgFIFO.IsEmpty())
      {
        m_interConnectFIFO->m_RespMsgFIFO.PopElement();
      }
      msg.cycle = m_arbiCycle;
      m_interConnectFIFO->m_RespMsgFIFO.InsertElement(msg);
    }
  }

  bool BusArbiter::FcFsWriteBackCheckInsert(uint16_t core_idx, uint64_t addr, bool CheckOnly, BusIfFIFO::BusRespMsg &txResp)
  {
    bool PendingReq = false;
    std::list<BusIfFIFO *>::iterator it1 = m_busIfFIFO.begin();
    std::advance(it1, core_idx);

    // get current size of the RxResp queue
    int pendingQueueSize = (*it1)->m_txRespFIFO.GetQueueSize();
    BusIfFIFO::BusRespMsg pendingWbMsg;

    // iterate over buffer to check if the WB message is there
    for (int i = 0; i < pendingQueueSize; i++)
    {
      pendingWbMsg = (*it1)->m_txRespFIFO.GetFrontElement();
      // Remove message from the busResp buffer
      (*it1)->m_txRespFIFO.PopElement();
      if (((pendingWbMsg.addr >> (int)log2(m_cacheBlkSize)) == (addr >> (int)log2(m_cacheBlkSize))) && PendingReq == false)
      {
        if (CheckOnly == true)
        {
          (*it1)->m_txRespFIFO.InsertElement(pendingWbMsg);
        }
        PendingReq = true;
        txResp = pendingWbMsg;
      }
      else
      {
        (*it1)->m_txRespFIFO.InsertElement(pendingWbMsg);
      }
    }

    // process pending response
    if (PendingReq == true && CheckOnly == false)
    {
      if (m_logFileGenEnable)
      {
        std::cout << "\nBusArbiter: Cpu " << (*it1)->m_interface_id << " granted TDM response slot =============================================================== " << m_arbiCycle << "\n\n";
      }
      SendData(txResp, AGENT::CORE);

      if (txResp.reqCoreId == m_sharedCacheBusIfFIFO->m_interface_id || txResp.dualTrans == true)
      {
        if (m_logFileGenEnable)
        {
          std::cout << "\nBusArbiter: Cpu " << (*it1)->m_interface_id << " Sent Data to Shared Mem =============================================================== " << m_arbiCycle << "\n\n";
        }
        SendData(txResp, AGENT::SHAREDMEM);
      }
      SendData(txResp, AGENT::INTERCONNECT);
      return true;
    }
    return PendingReq;
  }

  bool BusArbiter::FcFsMemCheckInsert(uint16_t coreId, uint64_t addr, bool CheckOnly = true, bool SkipAddrCheck = false)
  {
    bool PendingReq = false;
    int pendingQueueSize = m_sharedCacheBusIfFIFO->m_txRespFIFO.GetQueueSize();
    BusIfFIFO::BusRespMsg pendingWbMsg, txResp;

    for (int i = 0; i < pendingQueueSize; i++)
    {
      pendingWbMsg = m_sharedCacheBusIfFIFO->m_txRespFIFO.GetFrontElement();
      m_sharedCacheBusIfFIFO->m_txRespFIFO.PopElement();
      if (pendingWbMsg.reqCoreId == coreId && (pendingWbMsg.addr == addr || SkipAddrCheck == true))
      {
        if (CheckOnly == true)
        {
          m_sharedCacheBusIfFIFO->m_txRespFIFO.InsertElement(pendingWbMsg);
        }
        PendingReq = true;
        txResp = pendingWbMsg;
        break;
      }
      else
      {
        m_sharedCacheBusIfFIFO->m_txRespFIFO.InsertElement(pendingWbMsg);
      }
    }
    if (PendingReq == true && CheckOnly == false)
    {
      if (m_logFileGenEnable)
      {
        std::cout << "BusArbiter: SharedMem granted TDM response slot ============================================================ " << m_arbiCycle << "\n\n";
      }

      SendData(txResp, AGENT::CORE);

      SendData(txResp, AGENT::INTERCONNECT);

      return true;
    }
    return PendingReq;
  }

  bool BusArbiter::CheckPendingFCFSReq(BusIfFIFO::BusReqMsg &txMsg, bool ChkOnly = true)
  {
    double arrivalTime = 0;
    uint16_t core_idx = 0;
    uint16_t TargetCore = 0;
    bool PendingTxReq = false;

    for (std::list<BusIfFIFO *>::iterator it1 = m_busIfFIFO.begin(); it1 != m_busIfFIFO.end(); it1++)
    {
      if ((*it1)->m_txMsgFIFO.IsEmpty() == false)
      {
        txMsg = (*it1)->m_txMsgFIFO.GetFrontElement();
        if ((PendingTxReq == false) || arrivalTime > txMsg.timestamp)
        {
          arrivalTime = txMsg.timestamp;
          TargetCore = core_idx;
        }
        PendingTxReq = true;
      }
      core_idx++;
    }

    if (PendingTxReq)
    {
      std::list<BusIfFIFO *>::iterator it2 = m_busIfFIFO.begin();
      std::advance(it2, TargetCore);
      txMsg = (*it2)->m_txMsgFIFO.GetFrontElement();
      // Remove message from the busResp buffer
      if (ChkOnly == false)
      {
        (*it2)->m_txMsgFIFO.PopElement();
      }
    }
    return PendingTxReq;
  }

  bool BusArbiter::CheckPendingFCFSResp(BusIfFIFO::BusRespMsg &txMsg, bool ChkOnly = true)
  {
    double arrivalTime = 0;
    uint16_t core_idx = 0;
    uint16_t TargetCore = 0;
    bool PendingTxReq = false;

    for (std::list<BusIfFIFO *>::iterator it1 = m_busIfFIFO.begin(); it1 != m_busIfFIFO.end(); it1++)
    {
      if ((*it1)->m_txRespFIFO.IsEmpty() == false)
      {
        txMsg = (*it1)->m_txRespFIFO.GetFrontElement();
        if ((PendingTxReq == false) || arrivalTime > txMsg.timestamp)
        {
          arrivalTime = txMsg.timestamp;
          TargetCore = core_idx;
        }
        PendingTxReq = true;
      }
      core_idx++;
    }

    BusIfFIFO::BusRespMsg txMsgMem;
    if (m_sharedCacheBusIfFIFO->m_txRespFIFO.IsEmpty() == false)
    {
      txMsgMem = m_sharedCacheBusIfFIFO->m_txRespFIFO.GetFrontElement();
      if ((PendingTxReq == false) || arrivalTime > txMsgMem.timestamp)
      {
        arrivalTime = txMsgMem.timestamp;
        TargetCore = m_sharedCacheBusIfFIFO->m_interface_id;
      }
      PendingTxReq = true;
    }

    if (PendingTxReq)
    {
      if (TargetCore == m_sharedCacheBusIfFIFO->m_interface_id)
      {
        txMsg = txMsgMem;
        if (ChkOnly == false)
        {
          m_sharedCacheBusIfFIFO->m_txRespFIFO.PopElement();
        }
      }
      else
      {
        std::list<BusIfFIFO *>::iterator it2 = m_busIfFIFO.begin();
        std::advance(it2, TargetCore);
        txMsg = (*it2)->m_txRespFIFO.GetFrontElement();
        // Remove message from the busResp buffer
        if (ChkOnly == false)
        {
          (*it2)->m_txRespFIFO.PopElement();
        }
      }

      if (ChkOnly == false)
      {
        if (m_logFileGenEnable)
        {
          if (TargetCore == m_sharedCacheBusIfFIFO->m_interface_id)
          {
            std::cout << "\nBusArbiter: SharedMem " << m_sharedCacheBusIfFIFO->m_interface_id << " granted TDM response slot =============================================================== " << m_arbiCycle << "\n\n";
          }
          else
          {
            std::cout << "\nBusArbiter: Cpu " << TargetCore << " granted TDM response slot =============================================================== " << m_arbiCycle << "\n\n";
          }
        }

        SendData(txMsg, AGENT::CORE);

        if (txMsg.reqCoreId == m_sharedCacheBusIfFIFO->m_interface_id || txMsg.dualTrans == true)
        {
          if (m_logFileGenEnable)
          {
            std::cout << "\nBusArbiter: Cpu " << TargetCore << " Sent Data to Shared Mem =============================================================== " << m_arbiCycle << "\n\n";
          }
          SendData(txMsg, AGENT::SHAREDMEM);
        }

        SendData(txMsg, AGENT::INTERCONNECT);
      }
    }
    return PendingTxReq;
  }

  bool BusArbiter::CheckPendingReq(uint16_t core_idx, BusIfFIFO::BusReqMsg &txMsg, bool CheckOnly = false)
  {
    std::list<BusIfFIFO *>::iterator it1 = m_busIfFIFO.begin();
    std::advance(it1, core_idx);
    bool PendingTxReq = false;

    if (!(*it1)->m_txMsgFIFO.IsEmpty())
    {
      PendingTxReq = true;
      txMsg = (*it1)->m_txMsgFIFO.GetFrontElement();
      if (PendingTxReq && CheckOnly == false)
      {
        // Remove message from the busResp buffer
        (*it1)->m_txMsgFIFO.PopElement();
      }
    }
    return PendingTxReq;
  }

  bool BusArbiter::CheckPendingMessage(uint16_t core_idx, BusIfFIFO::BusReqMsg &txMsg, bool &demanding_turn)
  {
    std::list<BusIfFIFO *>::iterator it1 = m_busIfFIFO.begin();
    std::advance(it1, core_idx);
    BusIfFIFO_PMSI *ptr_pmsi = dynamic_cast<BusIfFIFO_PMSI *>(*it1);
    demanding_turn = true;

    if (ptr_pmsi != NULL)
    {
      demanding_turn = ptr_pmsi->demanding_turn;
      ptr_pmsi->demanding_turn = !ptr_pmsi->demanding_turn; //This to mimic the intra-core TDM arbiter
    }

    for (int i = 0; i < 2; i++, demanding_turn = !demanding_turn) //To Create work-conserving TDM behavior
    {
      if (!(*it1)->m_txMsgFIFO.IsEmpty() && demanding_turn)
      {
        txMsg = (*it1)->m_txMsgFIFO.GetFrontElement();
        (*it1)->m_txMsgFIFO.PopElement();
        return true;
      }
      else if(!m_sharedCacheBusIfFIFO->m_txRespFIFO.IsEmpty() && demanding_turn) //check if there is a reponse for the selected core (for PMSI/PMESI or protocols without c2c)
      {
        for (int i = 0; i < m_sharedCacheBusIfFIFO->m_txRespFIFO.GetQueueSize(); i++)
        {
          BusIfFIFO::BusRespMsg resp_msg = m_sharedCacheBusIfFIFO->m_txRespFIFO.GetFrontElement();
          m_sharedCacheBusIfFIFO->m_txRespFIFO.PopElement();
          m_sharedCacheBusIfFIFO->m_txRespFIFO.InsertElement(resp_msg);
          if (resp_msg.reqCoreId == core_idx)
            return false;
        }
      }
      else if (!demanding_turn && ptr_pmsi != NULL && !ptr_pmsi->m_txMsgFIFO_non_demanding.IsEmpty())
      {
        txMsg = ptr_pmsi->m_txMsgFIFO_non_demanding.GetFrontElement();
        ptr_pmsi->m_txMsgFIFO_non_demanding.PopElement();
        return true;
      }
    }

    demanding_turn = true; //return it back to true if no message returned
    return false;
  }

  bool BusArbiter::CheckPendingWB(uint16_t core_idx, BusIfFIFO::BusRespMsg &wbMsg, bool CheckOnly = false)
  {
    std::list<BusIfFIFO *>::iterator it1 = m_busIfFIFO.begin();
    std::advance(it1, core_idx);
    bool PendingWbFlag = false;

    if (!(*it1)->m_txRespFIFO.IsEmpty())
    {
      PendingWbFlag = true;
      wbMsg = (*it1)->m_txRespFIFO.GetFrontElement();
      if (PendingWbFlag && CheckOnly == false)
      {
        // Remove message from the busResp buffer
        (*it1)->m_txRespFIFO.PopElement();
      }
    }
    return PendingWbFlag;
  }

  bool BusArbiter::CheckPendingPutM(BusIfFIFO::BusReqMsg reqMsg, BusIfFIFO::BusReqMsg &putmReq)
  {
    BusIfFIFO::BusReqMsg txreq;
    int pendingQueueSize;
    bool PendingPutmReq = false;

    for (std::list<BusIfFIFO *>::iterator it1 = m_busIfFIFO.begin(); it1 != m_busIfFIFO.end() && (PendingPutmReq == false); it1++)
    {
      // check if core has pending requests
      pendingQueueSize = (*it1)->m_txMsgFIFO.GetQueueSize();
      for (int i = 0; i < pendingQueueSize; i++)
      {
        txreq = (*it1)->m_txMsgFIFO.GetFrontElement();
        // Remove message from the busReq buffer
        (*it1)->m_txMsgFIFO.PopElement();
        if (txreq.cohrMsgId == SNOOPPrivCohTrans::PutMTrans && (txreq.addr == reqMsg.addr) && (txreq.reqCoreId == reqMsg.reqCoreId))
        {
          putmReq = txreq;
          PendingPutmReq = true;
        }
        // else if (txreq.cohrMsgId == SNOOPPrivCohTrans::PutMTrans &&
        //          ((txreq.addr >> (int)log2(m_cacheBlkSize)) == (reqMsg.addr >> (int)log2(m_cacheBlkSize))) &&
        //          (txreq.msgId == 0))
        // {
        //   putmReq = txreq;
        //   putmReq.addr = reqMsg.addr;
        //   putmReq.msgId = reqMsg.msgId;
        //   putmReq.reqCoreId = reqMsg.reqCoreId;
        //   PendingPutmReq = true;
        // }
        else
        {
          (*it1)->m_txMsgFIFO.InsertElement(txreq);
        }
      }
    }
    return PendingPutmReq;
  }

  bool BusArbiter::InsertOnReqBus(BusIfFIFO::BusReqMsg txMsg)
  {
    if (m_logFileGenEnable)
    {
      if (txMsg.cohrMsgId == SNOOPPrivCohTrans::PutMTrans)
      {
        std::cout << "\nBusArbiter: Cpu " << txMsg.wbCoreId << " granted TDM request slot =============================================================== " << m_arbiCycle << std::endl;
      }
      else
      {
        std::cout << "\nBusArbiter: Cpu " << txMsg.reqCoreId << " granted TDM request slot =============================================================== " << m_arbiCycle << std::endl;
      }
    }

    Logger::getLogger()->updateRequest(txMsg.msgId, Logger::EntryId::REQ_BUS_CHECKPOINT);

    if(txMsg.msgId == 0)
      Logger::getLogger()->registerReplacementMessage(txMsg.wbCoreId);

    // broadcast requests to all cores (snooping based)
    for (std::list<BusIfFIFO *>::iterator it2 = m_busIfFIFO.begin(); it2 != m_busIfFIFO.end(); it2++)
    {
      if (!(*it2)->m_rxMsgFIFO.IsFull())
      {
        (*it2)->m_rxMsgFIFO.InsertElement(txMsg);
      }
      else
      {
        if (m_logFileGenEnable)
        {
          std::cout << "BusArbiter: cannot insert new messages into PrivCache Ctrl buffers" << std::endl;
        }
        exit(0);
      }
    }
    // send request to Shared Mem controller as well
    if (!m_sharedCacheBusIfFIFO->m_rxMsgFIFO.IsFull())
    {
      m_sharedCacheBusIfFIFO->m_rxMsgFIFO.InsertElement(txMsg);
    }
    else
    {
      if (m_logFileGenEnable)
      {
        std::cout << "BusArbiter: cannot insert new messages into SharedMem RxMsg buffers" << std::endl;
      }
      exit(0);
    }
    // send message to Interconnect FIFO
    if (!m_interConnectFIFO->m_ReqMsgFIFO.IsEmpty())
    {
      m_interConnectFIFO->m_ReqMsgFIFO.PopElement();
    }
    txMsg.cycle = m_arbiCycle;
    m_interConnectFIFO->m_ReqMsgFIFO.InsertElement(txMsg);
    return true;
  }

  void BusArbiter::PMSI_TDM_ReqBus()
  {
    BusIfFIFO::BusReqMsg tempPutmMsg;
    if (m_PndReq)
    {
      InsertOnReqBus(m_ReqBusMsg);
      Simulator::Schedule(NanoSeconds(m_dt * m_reqclks / 2), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
      m_PndPutMChk = true;
      m_PndReq = false;
      return;
    }

    if (m_PndPutMChk)
    {
      if (CheckPendingPutM(m_ReqBusMsg, tempPutmMsg))
      {
        InsertOnReqBus(tempPutmMsg);
        m_GlobalReqFIFO.InsertElement(tempPutmMsg);
        if (m_logFileGenEnable)
        {
          std::cout << "BusArbiter: Insert Put(M) on the Bus from Core " << tempPutmMsg.wbCoreId << "============================================================\n\n";
        }
      }

      if (m_ReqBusMsg.cohrMsgId == SNOOPPrivCohTrans::PutMTrans)
      {
        int pendingQueueSize = m_GlobalReqFIFO.GetQueueSize();
        bool InsertionDone = false;
        BusIfFIFO::BusReqMsg txResp;
        for (int i = 0; i < pendingQueueSize; i++)
        {
          txResp = m_GlobalReqFIFO.GetFrontElement();
          // Remove message from the busResp buffer
          m_GlobalReqFIFO.PopElement();

          if (InsertionDone == false && (txResp.cohrMsgId == SNOOPPrivCohTrans::GetSTrans || SNOOPPrivCohTrans::GetMTrans) &&
              ((txResp.addr >> (int)log2(m_cacheBlkSize)) == (m_ReqBusMsg.addr >> (int)log2(m_cacheBlkSize))))
          {
            m_GlobalReqFIFO.InsertElement(m_ReqBusMsg);
            m_GlobalReqFIFO.InsertElement(txResp);
            InsertionDone = true;
          }
          else
          {
            m_GlobalReqFIFO.InsertElement(txResp);
          }
        }

        if (InsertionDone == false)
        {
          m_GlobalReqFIFO.InsertElement(m_ReqBusMsg);
        }
      }
      else if (m_ReqBusMsg.cohrMsgId != SNOOPPrivCohTrans::UpgTrans)
      {
        m_GlobalReqFIFO.InsertElement(m_ReqBusMsg);
      }

      // advance TDM slot pointer
      m_reqCoreCnt = (m_reqCoreCnt == m_cpuCore - 1) ? 0 : (m_reqCoreCnt + 1);
      m_PndPutMChk = false;
    }

    for (int i = 0; i < m_cpuCore && m_PndReq == false; i++)
    {
      m_PndReq = CheckPendingReq(m_reqCoreCnt, m_ReqBusMsg);
      if (!m_PndReq)
        m_reqCoreCnt = (m_reqCoreCnt == m_cpuCore - 1) ? 0 : (m_reqCoreCnt + 1);
    }

    if (m_PndReq)
    {
      // wait one TDM Request slot
      Simulator::Schedule(NanoSeconds(m_dt * m_reqclks / 2), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
    }
    else
    {
      // wait one clock cycle and check again
      Simulator::Schedule(NanoSeconds(m_dt), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
    }
  } // void BusArbiter::PMSI_TDM_ReqBus()

  void BusArbiter::TDM_PCC()
  {
    int m_slot_width = m_respclks + m_reqclks;
    int clk_in_slot = m_arbiCycle % m_slot_width;

    static BusIfFIFO::BusReqMsg *req_msg_ptr = NULL;
    static BusIfFIFO::BusRespMsg *resp_msg_ptr = NULL;
    static uint64_t m_selected_core = 0;
    static bool falling_edge = true;

    if (falling_edge)
      this->selectCore_TDM(&m_selected_core, clk_in_slot);

    if (clk_in_slot == 0 && falling_edge) //Check request from selected core
    {
      req_msg_ptr = new BusIfFIFO::BusReqMsg;
      if (!CheckPendingReq(m_selected_core, *req_msg_ptr))
      {
        delete req_msg_ptr;
        req_msg_ptr = NULL;
      }
    }
    else if (clk_in_slot == (m_reqclks - 1) && req_msg_ptr != NULL && falling_edge) //Send the request (if avaiable) on the bus
    {
      InsertOnReqBus(*req_msg_ptr);
      delete req_msg_ptr;
      req_msg_ptr = NULL;
    }
    else if (clk_in_slot == m_reqclks && falling_edge) //Send the response
    {
      resp_msg_ptr = new BusIfFIFO::BusRespMsg;
      if (!this->getResponse(m_selected_core, resp_msg_ptr))
      {
        delete resp_msg_ptr;
        resp_msg_ptr = NULL;
      }
    }
    else if (clk_in_slot == (m_respclks + m_reqclks - 1) && resp_msg_ptr != NULL && falling_edge) //Send response
    {
      if (resp_msg_ptr->reqCoreId == m_sharedCacheBusIfFIFO->m_interface_id)
      {
        SendData(*resp_msg_ptr, AGENT::SHAREDMEM);
      }
      else
      {
        SendData(*resp_msg_ptr, AGENT::CORE);
        SendData(*resp_msg_ptr, AGENT::INTERCONNECT);
        if (resp_msg_ptr->dualTrans)
          SendData(*resp_msg_ptr, AGENT::SHAREDMEM);
      }

      delete resp_msg_ptr;
      resp_msg_ptr = NULL;
    }

    falling_edge = !falling_edge;
    Simulator::Schedule(NanoSeconds(m_dt / 2), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
  }

  void BusArbiter::RR_PCC()
  {
    int m_slot_width = m_respclks + m_reqclks;

    static int clk_in_slot = 0;
    static BusIfFIFO::BusReqMsg *req_msg_ptr = NULL;
    static BusIfFIFO::BusRespMsg *resp_msg_ptr = NULL;
    static uint64_t m_selected_core = 0;
    static bool falling_edge = false;

    if (clk_in_slot == 0 && falling_edge) //Check request from selected core
    {
      for (int i = 0; i < m_cpuCore && req_msg_ptr == NULL; i++)
      {
        selectCore_RR(&m_selected_core);
        req_msg_ptr = new BusIfFIFO::BusReqMsg;
        if (!CheckPendingReq(m_selected_core, *req_msg_ptr))
        {
          delete req_msg_ptr;
          req_msg_ptr = NULL;
        }
      }
    }
    else if (clk_in_slot == (m_reqclks - 1) && req_msg_ptr != NULL && falling_edge) //Send the request (if avaiable) on the bus
    {
      InsertOnReqBus(*req_msg_ptr);
      delete req_msg_ptr;
      req_msg_ptr = NULL;
    }
    else if (clk_in_slot == m_reqclks && falling_edge) //Send the response
    {
      resp_msg_ptr = new BusIfFIFO::BusRespMsg;
      if (!this->getResponse(m_selected_core, resp_msg_ptr))
      {
        delete resp_msg_ptr;
        resp_msg_ptr = NULL;
      }
    }
    else if (clk_in_slot == (m_respclks + m_reqclks - 1) && resp_msg_ptr != NULL && falling_edge) //Send response
    {
      if (resp_msg_ptr->reqCoreId == m_sharedCacheBusIfFIFO->m_interface_id)
      {
        SendData(*resp_msg_ptr, AGENT::SHAREDMEM);
      }
      else
      {
        SendData(*resp_msg_ptr, AGENT::CORE);
        SendData(*resp_msg_ptr, AGENT::INTERCONNECT);
        if (resp_msg_ptr->dualTrans)
          SendData(*resp_msg_ptr, AGENT::SHAREDMEM);
      }

      delete resp_msg_ptr;
      resp_msg_ptr = NULL;
    }

    if(falling_edge)
    {
      if(clk_in_slot == 0 && req_msg_ptr == NULL) //don't count if no request found to recheck again in the following clk
        clk_in_slot = 0;
      else if(clk_in_slot == m_reqclks && resp_msg_ptr == NULL)
        clk_in_slot = 0;
      else
        clk_in_slot = (clk_in_slot + 1) % m_slot_width;
    }  

    falling_edge = !falling_edge;
    Simulator::Schedule(NanoSeconds(m_dt / 2), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
  }

  void BusArbiter::FCFS_PCC()
  {
    int m_slot_width = m_respclks + m_reqclks;

    static int clk_in_slot = 0;
    static BusIfFIFO::BusReqMsg *req_msg_ptr = NULL;
    static BusIfFIFO::BusRespMsg *resp_msg_ptr = NULL;
    static uint64_t m_selected_core = 0;
    static bool falling_edge = false;

    if (clk_in_slot == 0 && falling_edge) //Check request from selected core
    {
      selectCore_FCFS(&m_selected_core);
      req_msg_ptr = new BusIfFIFO::BusReqMsg;
      if (!CheckPendingReq(m_selected_core, *req_msg_ptr))
      {
        delete req_msg_ptr;
        req_msg_ptr = NULL;
      }
    }
    else if (clk_in_slot == (m_reqclks - 1) && req_msg_ptr != NULL && falling_edge) //Send the request (if avaiable) on the bus
    {
      InsertOnReqBus(*req_msg_ptr);
      delete req_msg_ptr;
      req_msg_ptr = NULL;
    }
    else if (clk_in_slot == m_reqclks && falling_edge) //Send the response
    {
      resp_msg_ptr = new BusIfFIFO::BusRespMsg;
      if (!this->getResponse(m_selected_core, resp_msg_ptr))
      {
        delete resp_msg_ptr;
        resp_msg_ptr = NULL;
      }
    }
    else if (clk_in_slot == (m_respclks + m_reqclks - 1) && resp_msg_ptr != NULL && falling_edge) //Send response
    {
      if (resp_msg_ptr->reqCoreId == m_sharedCacheBusIfFIFO->m_interface_id)
      {
        SendData(*resp_msg_ptr, AGENT::SHAREDMEM);
      }
      else
      {
        SendData(*resp_msg_ptr, AGENT::CORE);
        SendData(*resp_msg_ptr, AGENT::INTERCONNECT);
        if (resp_msg_ptr->dualTrans)
          SendData(*resp_msg_ptr, AGENT::SHAREDMEM);
      }

      delete resp_msg_ptr;
      resp_msg_ptr = NULL;
    }

    if(falling_edge)
    {
      if(clk_in_slot == 0 && req_msg_ptr == NULL) //don't count if no request found to recheck again in the following clk
        clk_in_slot = 0;
      else if(clk_in_slot == m_reqclks && resp_msg_ptr == NULL)
        clk_in_slot = 0;
      else
        clk_in_slot = (clk_in_slot + 1) % m_slot_width;
    }  

    falling_edge = !falling_edge;
    Simulator::Schedule(NanoSeconds(m_dt / 2), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
  }

  void BusArbiter::TDM_PMSI()
  {
    int m_slot_width = m_respclks + m_reqclks;
    int clk_in_slot = m_arbiCycle % m_slot_width;

    static BusIfFIFO::BusReqMsg *req_msg_ptr = NULL;
    static BusIfFIFO::BusRespMsg *resp_msg_ptr = NULL;
    static uint64_t m_selected_core = 0;
    static bool falling_edge = true;
    static bool demanding_turn = true;

    if (falling_edge)
      this->selectCore_TDM(&m_selected_core, clk_in_slot);

    if (clk_in_slot == 0 && falling_edge) //Check request from selected core
    {
      req_msg_ptr = new BusIfFIFO::BusReqMsg;
      if (!CheckPendingMessage(m_selected_core, *req_msg_ptr, demanding_turn))
      {
        delete req_msg_ptr;
        req_msg_ptr = NULL;
      }
    }
    else if (clk_in_slot == (m_reqclks - 1) && req_msg_ptr != NULL && falling_edge) //Send the request (if avaiable) on the bus
    {
      InsertOnReqBus(*req_msg_ptr);
      delete req_msg_ptr;
      req_msg_ptr = NULL;
    }
    else if (clk_in_slot == m_reqclks && falling_edge) //Send the response
    {
      resp_msg_ptr = new BusIfFIFO::BusRespMsg;
      if (!demanding_turn)
      {
        if (!this->conductWriteback(m_selected_core, resp_msg_ptr))
        {
          std::cout << "TDM_PMSI: Error non-demanding PutM without data!!" << std::endl;
          exit(0);
        }
      }
      else if (!this->getResponse(m_selected_core, resp_msg_ptr))
      {
        delete resp_msg_ptr;
        resp_msg_ptr = NULL;
      }
    }
    else if (clk_in_slot == (m_respclks + m_reqclks - 1) && resp_msg_ptr != NULL && falling_edge) //Send response
    {
      if (resp_msg_ptr->reqCoreId == m_sharedCacheBusIfFIFO->m_interface_id)
      {
        SendData(*resp_msg_ptr, AGENT::SHAREDMEM);
      }
      else
      {
        SendData(*resp_msg_ptr, AGENT::CORE);
        SendData(*resp_msg_ptr, AGENT::INTERCONNECT);
        if (resp_msg_ptr->dualTrans)
          SendData(*resp_msg_ptr, AGENT::SHAREDMEM);
      }

      delete resp_msg_ptr;
      resp_msg_ptr = NULL;
    }

    falling_edge = !falling_edge;
    Simulator::Schedule(NanoSeconds(m_dt / 2), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
  }

  void BusArbiter::selectCore_TDM(uint64_t *selected_core, int clk_in_slot)
  {
    if (clk_in_slot == 0)
      *selected_core = (*selected_core + 1) % m_cpuCore;
  }

  void BusArbiter::selectCore_RR(uint64_t *selected_core)
  {
    static int index = 0;
    std::vector<int> core_distribution;

    if(m_bus_arb == BusARBType::UNIFIED_RR_ARB)
      core_distribution = {0, 1, 2, 3}; //RR
    else if(m_bus_arb == BusARBType::UNIFIED_WRR_ARB)
      core_distribution = {0, 0, 0, 0, 1, 1, 2, 3}; //WRR
    else if(m_bus_arb == BusARBType::UNIFIED_HRR_ARB)
      core_distribution = {0, 1, 0, 2, 0, 1, 0, 3}; //HRR
    
    *selected_core = core_distribution[index];
    index = (index + 1) % (core_distribution.size());
  }

  void BusArbiter::selectCore_FCFS(uint64_t *selected_core)
  {
    std::list<BusIfFIFO *>::iterator itr = m_busIfFIFO.begin();
    BusIfFIFO::BusReqMsg first_msg;
    first_msg.cycle = (uint64_t)-1; //Max value

    for(int i = 0; i < (int)m_busIfFIFO.size(); i++)
    {
      if (!(*itr)->m_txMsgFIFO.IsEmpty())
      {
        BusIfFIFO::BusReqMsg temp_msg = (*itr)->m_txMsgFIFO.GetFrontElement();
        if (temp_msg.cycle < first_msg.cycle)
        {
          first_msg = temp_msg;
          *selected_core = i;
        }
      }
      itr++;
    }
  }

  bool BusArbiter::getResponse(uint16_t coreId, BusIfFIFO::BusRespMsg *resp_msg)
  {
    //Data from LLC
    for (int i = 0; i < m_sharedCacheBusIfFIFO->m_txRespFIFO.GetQueueSize(); i++)
    {
      *resp_msg = m_sharedCacheBusIfFIFO->m_txRespFIFO.GetFrontElement();
      m_sharedCacheBusIfFIFO->m_txRespFIFO.PopElement();
      if (resp_msg->reqCoreId == coreId)
        return true;
      else
        m_sharedCacheBusIfFIFO->m_txRespFIFO.InsertElement(*resp_msg);
    }

    //Data from a fellow cache
    for (std::list<BusIfFIFO *>::iterator itr = m_busIfFIFO.begin(); itr != m_busIfFIFO.end(); itr++)
    {
      for (int i = 0; i < (*itr)->m_txRespFIFO.GetQueueSize(); i++)
      {
        *resp_msg = (*itr)->m_txRespFIFO.GetFrontElement();
        (*itr)->m_txRespFIFO.PopElement();

        if (resp_msg->reqCoreId == coreId)
          return true;
        else
          (*itr)->m_txRespFIFO.InsertElement(*resp_msg);
      }
    }

    //Write Back
    std::list<BusIfFIFO *>::iterator it1 = m_busIfFIFO.begin();
    std::advance(it1, coreId);

    if (!(*it1)->m_txRespFIFO.IsEmpty())
    {
      *resp_msg = (*it1)->m_txRespFIFO.GetFrontElement();
      (*it1)->m_txRespFIFO.PopElement();
      return true;
    }

    return false;
  }

  bool BusArbiter::conductWriteback(uint16_t coreId, BusIfFIFO::BusRespMsg *resp_msg)
  {
    std::list<BusIfFIFO *>::iterator it1 = m_busIfFIFO.begin();
    std::advance(it1, coreId);

    if (!(*it1)->m_txRespFIFO.IsEmpty())
    {
      *resp_msg = (*it1)->m_txRespFIFO.GetFrontElement();
      (*it1)->m_txRespFIFO.PopElement();
      return true;
    }

    return false;
  }

  // Unified TDM bus
  void BusArbiter::Unified_TDM_PMSI_Bus2()
  {

    BusIfFIFO::BusReqMsg tempPutmMsg;
    bool PutMFromOtherCore;
    uint16_t IdleSlotWidth;

    // do write back to memory or core
    // global queue actions (TDM response slot)
    if (m_PndMemResp || m_PndWB)
    {
      if (m_PndMemResp)
      {
        FcFsMemCheckInsert(m_reqCoreCnt, 0, false, true);
      }
      else
      {
        if (m_PendWBMsg.reqCoreId == m_sharedCacheBusIfFIFO->m_interface_id || m_cach2Cache == false)
        {
          SendData(m_PendWBMsg, AGENT::SHAREDMEM);
        }
        else
        {
          SendData(m_PendWBMsg, AGENT::CORE);
          if (m_PendWBMsg.dualTrans)
            SendData(m_PendWBMsg, AGENT::SHAREDMEM);
        }
      }

      m_PndMemResp = false;
      m_PndWB = false;

      m_reqCoreCnt = (m_reqCoreCnt == m_cpuCore - 1) ? 0 : (m_reqCoreCnt + 1);
      Simulator::Schedule(NanoSeconds(m_dt * 2), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
      return;
    }
    else if (m_PndReq)
    {
      InsertOnReqBus(m_ReqBusMsg);

      Simulator::Schedule(NanoSeconds(m_dt * m_reqclks / 2), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
      m_PndPutMChk = true;
      m_PndReq = false;
      return;
    }
    else if (m_PndPutMChk)
    {
      PutMFromOtherCore = CheckPendingPutM(m_ReqBusMsg, tempPutmMsg);
      if (PutMFromOtherCore)
      {
        InsertOnReqBus(tempPutmMsg);
        if (m_logFileGenEnable)
        {
          std::cout << "BusArbiter: Insert Put(M) on the Bus from Core " << tempPutmMsg.wbCoreId << "============================================================\n\n";
        }
        m_reqCoreCnt = (m_reqCoreCnt == m_cpuCore - 1) ? 0 : (m_reqCoreCnt + 1);
      }
      else
      {
        m_DirectTransfer = true;
      }

      m_PndPutMChk = false;

      Simulator::Schedule(NanoSeconds(m_dt * (m_respclks - m_reqclks)), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
      return;
    }
    else if (m_DirectTransfer)
    {
      if (m_ReqBusMsg.cohrMsgId == SNOOPPrivCohTrans::PutMTrans)
      {
        FcFsWriteBackCheckInsert(m_reqCoreCnt, m_ReqBusMsg.addr, false, m_PendResp);
        if (m_logFileGenEnable)
        {
          std::cout << "BusArbiter: Direct Core replacement from Core: " << m_reqCoreCnt << " ============================================== " << m_arbiCycle << "\n\n";
        }
      }
      else
      {
        if (!FcFsMemCheckInsert(m_reqCoreCnt, m_ReqBusMsg.addr, false))
        {
          if (m_logFileGenEnable)
          {
            std::cout << "BusArbiter: Not ready direct Memory Transfer to Core: " << m_reqCoreCnt << " ============================================== " << m_arbiCycle << "\n\n";
          }
        }
      }
      m_DirectTransfer = false;

      m_reqCoreCnt = (m_reqCoreCnt == m_cpuCore - 1) ? 0 : (m_reqCoreCnt + 1);
    }
    else if (m_IdleSlot)
    {
      m_IdleSlot = false;
      m_reqCoreCnt = (m_reqCoreCnt == m_cpuCore - 1) ? 0 : (m_reqCoreCnt + 1);
    }

    m_IdleSlot = true;

    m_PndReq = CheckPendingReq(m_reqCoreCnt, m_ReqBusMsg, true);

    m_PndWB = CheckPendingWB(m_reqCoreCnt, m_PendWBMsg, true);

    m_PndMemResp = FcFsMemCheckInsert(m_reqCoreCnt, 0, true, true);

    if ((m_PndReq || m_PndMemResp) && m_PndWB)
    {
      if (m_logFileGenEnable)
      {
        std::cout << "BusArbiter: Multiple request from core " << m_reqCoreCnt << " ===================================================" << m_arbiCycle << "\n\n";
      }
      if (m_ReqWbFlag[m_reqCoreCnt])
      {
        m_PndWB = false;
      }
      else
      {
        m_PndReq = false;
        m_PndMemResp = false;
      }
    }

    if (m_PndReq && m_PndMemResp)
    {
      m_PndReq = false;
    }

    if (m_PndReq || m_PndMemResp || m_PndWB)
    {
      m_IdleSlot = false;
      m_ReqWbFlag[m_reqCoreCnt] = (m_PndWB == true) ? true : false;
    }
    else
    {
      m_ReqWbFlag[m_reqCoreCnt] = true;
    }

    if (m_PndReq == true)
    {
      m_PndReq = CheckPendingReq(m_reqCoreCnt, m_ReqBusMsg, false);
      if (m_logFileGenEnable)
      {
        std::cout << "BusArbiter: Request Slot from core " << m_reqCoreCnt << " ===================================================" << m_arbiCycle << "\n\n";
      }
      Simulator::Schedule(NanoSeconds(m_dt * m_reqclks / 2), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
    }
    else
    {
      if (m_IdleSlot == true)
      {
        IdleSlotWidth = (m_workconserv == true) ? 1 : m_respclks;
        if (m_logFileGenEnable)
        {
          std::cout << "BusArbiter: Idle Slot for core " << m_reqCoreCnt << " ===================================================" << m_arbiCycle << "\n\n";
        }
        Simulator::Schedule(NanoSeconds(m_dt * IdleSlotWidth), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
      }
      else
      {
        if (m_PndWB)
        {
          m_PndWB = CheckPendingWB(m_reqCoreCnt, m_PendWBMsg, false);
        }
        Simulator::Schedule(NanoSeconds(m_dt * (m_respclks - 2)), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
      }
    }
  }

  // Unified TDM bus (PMSI only)
  void BusArbiter::Unified_TDM_PMSI_Bus()
  {
    // global queue msg
    BusIfFIFO::BusReqMsg glb_queue, tempPutmMsg;
    bool PutMFromOtherCore;
    uint16_t IdleSlotWidth;

    if (m_PndResp || m_FcFsPndMemResp)
    {
      glb_queue = m_GlobalReqFIFO.GetFrontElement();

      m_GlobalReqFIFO.PopElement();

      m_FcFsPndMemResp = (m_FcFsPndMemResp == true) ? FcFsMemCheckInsert(m_reqCoreCnt, glb_queue.addr, false) : FcFsWriteBackCheckInsert(m_reqCoreCnt, glb_queue.addr, false, m_PendResp);

      m_PndResp = false;
      m_FcFsPndMemResp = false;

      m_reqCoreCnt = (m_reqCoreCnt == m_cpuCore - 1) ? 0 : (m_reqCoreCnt + 1);
      Simulator::Schedule(NanoSeconds(m_dt * 2), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
      return;
    }
    else if (m_PndReq)
    {
      InsertOnReqBus(m_ReqBusMsg);

      Simulator::Schedule(NanoSeconds(m_dt * m_reqclks / 2), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
      m_PndPutMChk = true;
      m_PndReq = false;
      return;
    }
    else if (m_PndPutMChk)
    {
      PutMFromOtherCore = CheckPendingPutM(m_ReqBusMsg, tempPutmMsg);
      if (PutMFromOtherCore)
      {
        InsertOnReqBus(tempPutmMsg);
        m_GlobalReqFIFO.InsertElement(tempPutmMsg);
        m_GlobalReqFIFO.InsertElement(m_ReqBusMsg);
        if (m_logFileGenEnable)
        {
          std::cout << "BusArbiter: Insert Put(M) on the Bus from Core " << tempPutmMsg.wbCoreId << "============================================================\n\n";
        }
        m_reqCoreCnt = (m_reqCoreCnt == m_cpuCore - 1) ? 0 : (m_reqCoreCnt + 1);
      }
      else
      {
        m_DirectTransfer = true;
      }

      m_PndPutMChk = false;

      Simulator::Schedule(NanoSeconds(m_dt * (m_respclks - m_reqclks)), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
      return;
    }
    else if (m_DirectTransfer)
    {
      if (m_ReqBusMsg.cohrMsgId == SNOOPPrivCohTrans::PutMTrans)
      {
        FcFsWriteBackCheckInsert(m_reqCoreCnt, m_ReqBusMsg.addr, false, m_PendResp);
        if (m_logFileGenEnable)
        {
          std::cout << "BusArbiter: Direct Core replacement: " << m_reqCoreCnt << " ============================================== " << m_arbiCycle << "\n\n";
        }
      }
      else
      {
        if (!FcFsMemCheckInsert(m_reqCoreCnt, m_ReqBusMsg.addr, false))
        {
          m_GlobalReqFIFO.InsertElement(m_ReqBusMsg);
          if (m_logFileGenEnable)
          {
            std::cout << "BusArbiter: Direct Memory Transfer: " << m_reqCoreCnt << " ============================================== " << m_arbiCycle << "\n\n";
          }
        }
      }
      m_DirectTransfer = false;

      m_reqCoreCnt = (m_reqCoreCnt == m_cpuCore - 1) ? 0 : (m_reqCoreCnt + 1);
    }
    else if (m_IdleSlot)
    {
      m_IdleSlot = false;
      m_reqCoreCnt = (m_reqCoreCnt == m_cpuCore - 1) ? 0 : (m_reqCoreCnt + 1);
    }

    m_IdleSlot = true;

    if (!m_GlobalReqFIFO.IsEmpty())
    {
      glb_queue = m_GlobalReqFIFO.GetFrontElement();

      if (glb_queue.wbCoreId == m_reqCoreCnt && glb_queue.cohrMsgId == SNOOPPrivCohTrans::PutMTrans)
      {
        if (m_logFileGenEnable)
        {
          std::cout << "BusArbiter: Response Slot, Wb slot from core " << m_reqCoreCnt << "===================================================" << m_arbiCycle << "\n\n";
        }
        m_PndResp = true;
      }

      if (glb_queue.reqCoreId == m_reqCoreCnt && glb_queue.cohrMsgId != SNOOPPrivCohTrans::PutMTrans)
      {
        if (m_logFileGenEnable)
        {
          std::cout << "BusArbiter: Response Slot, Memory Transfer slot to core " << m_reqCoreCnt << "==================================================" << m_arbiCycle << "\n\n";
        }
        m_FcFsPndMemResp = true;
      }
    }

    m_PndReq = CheckPendingReq(m_reqCoreCnt, m_ReqBusMsg, true);

    if ((m_FcFsPndMemResp || m_PndResp) && m_PndReq)
    {
      if (m_ReqWbFlag[m_reqCoreCnt])
      {
        m_PndResp = false;
        m_FcFsPndMemResp = false;
      }
      else
      {
        m_PndReq = false;
      }
    }

    if (m_FcFsPndMemResp || m_PndResp || m_PndReq)
    {
      m_IdleSlot = false;
      m_ReqWbFlag[m_reqCoreCnt] = (m_PndResp == true) ? true : false;
    }

    if (m_PndReq == true)
    {
      m_PndReq = CheckPendingReq(m_reqCoreCnt, m_ReqBusMsg);
      if (m_logFileGenEnable)
      {
        std::cout << "BusArbiter: Request Slot from core " << m_reqCoreCnt << " ===================================================" << m_arbiCycle << "\n\n";
      }
      Simulator::Schedule(NanoSeconds(m_dt * m_reqclks / 2), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
    }
    else
    {
      if (m_IdleSlot == true)
      {
        IdleSlotWidth = (m_workconserv == true) ? 1 : m_respclks;
        if (m_logFileGenEnable)
        {
          std::cout << "BusArbiter: Idle Slot for core " << m_reqCoreCnt << " ===================================================" << m_arbiCycle << "\n\n";
        }
        Simulator::Schedule(NanoSeconds(m_dt * IdleSlotWidth), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
      }
      else
      {
        Simulator::Schedule(NanoSeconds(m_dt * (m_respclks - 2)), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
      }
    }

  } // void BusArbiter::Unified_TDM_PMSI_Bus

  void BusArbiter::PISCOT_MSI_TDM_ReqBus()
  {
    // send message on request bus
    if (m_PndReq)
    {
      InsertOnReqBus(m_ReqBusMsg);
      m_PndPutMChk = true;
      m_PndReq = false;
    }
    // insert message into service queue
    if (m_PndPutMChk)
    {
      // check if PutM need to be inserted
      if (m_ReqBusMsg.cohrMsgId == SNOOPPrivCohTrans::PutMTrans)
      {
        int pendingQueueSize = m_GlobalReqFIFO.GetQueueSize();
        bool InsertionDone = false;
        BusIfFIFO::BusReqMsg txResp;
        for (int i = 0; i < pendingQueueSize; i++)
        {
          txResp = m_GlobalReqFIFO.GetFrontElement();
          // Remove message from the busResp buffer
          m_GlobalReqFIFO.PopElement();

          if (InsertionDone == false && (txResp.cohrMsgId == SNOOPPrivCohTrans::GetSTrans || SNOOPPrivCohTrans::GetMTrans) &&
              ((txResp.addr >> (int)log2(m_cacheBlkSize)) == (m_ReqBusMsg.addr >> (int)log2(m_cacheBlkSize))))
          {
            InsertionDone = true;
          }
          // dequeuing
          m_GlobalReqFIFO.InsertElement(txResp);
        }
        if (InsertionDone == false)
        {
          m_GlobalReqFIFO.InsertElement(m_ReqBusMsg);
        }
      }
      else if (m_ReqBusMsg.cohrMsgId != SNOOPPrivCohTrans::UpgTrans)
      {
        m_GlobalReqFIFO.InsertElement(m_ReqBusMsg);
      }
      // advance TDM slot pointer
      m_reqCoreCnt = (m_reqCoreCnt == m_cpuCore - 1) ? 0 : (m_reqCoreCnt + 1);
      m_PndPutMChk = false;
    }

    // TDM request bus implementation
    for (int i = 0; i < m_cpuCore && m_PndReq == false; i++)
    {
      m_PndReq = CheckPendingReq(m_reqCoreCnt, m_ReqBusMsg);
      if (!m_PndReq)
        m_reqCoreCnt = (m_reqCoreCnt == m_cpuCore - 1) ? 0 : (m_reqCoreCnt + 1);
    }

    if (m_PndReq)
    {
      // wait one TDM Request slot
      Simulator::Schedule(NanoSeconds(m_dt * m_reqclks), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
    }
    else
    {
      // wait one clock cycle and check again
      Simulator::Schedule(NanoSeconds(m_dt), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
    }
  } // void BusArbiter::PISCOT_MSI_TDM_ReqBus()

  void BusArbiter::PMSI_OOO_TDM_ReqBus()
  {
    BusIfFIFO::BusReqMsg tempPutmMsg;
    if (m_PndReq)
    {
      InsertOnReqBus(m_ReqBusMsg);
      Simulator::Schedule(NanoSeconds(m_dt * m_reqclks / 2), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
      m_PndPutMChk = true;
      m_PndReq = false;
      return;
    }

    if (m_PndPutMChk)
    {
      // check if there is a pending PutM issued from other cores
      // This PutM has to broatcasted independed of the Bus
      // Arbiteration Policy.
      if (CheckPendingPutM(m_ReqBusMsg, tempPutmMsg))
      {
        InsertOnReqBus(tempPutmMsg);
        m_GlobalReqFIFO.InsertElement(tempPutmMsg);
        if (m_logFileGenEnable)
        {
          std::cout << "BusArbiter: Insert Put(M) on the Bus from Core " << tempPutmMsg.wbCoreId << "============================================================\n\n";
        }
      }

      // sort Resquest Messages
      if (m_ReqBusMsg.cohrMsgId == SNOOPPrivCohTrans::PutMTrans)
      {
        int pendingQueueSize = m_GlobalReqFIFO.GetQueueSize();
        bool InsertionDone = false;
        BusIfFIFO::BusReqMsg txResp;
        for (int i = 0; i < pendingQueueSize; i++)
        {
          txResp = m_GlobalReqFIFO.GetFrontElement();
          // Remove message from the busResp buffer
          m_GlobalReqFIFO.PopElement();

          if (InsertionDone == false && (txResp.cohrMsgId == SNOOPPrivCohTrans::GetSTrans || SNOOPPrivCohTrans::GetMTrans) &&
              ((txResp.addr >> (int)log2(m_cacheBlkSize)) == (m_ReqBusMsg.addr >> (int)log2(m_cacheBlkSize))))
          {
            m_GlobalReqFIFO.InsertElement(m_ReqBusMsg);
            m_GlobalReqFIFO.InsertElement(txResp);
            InsertionDone = true;
          }
          else
          {
            m_GlobalReqFIFO.InsertElement(txResp);
          }
        }

        if (InsertionDone == false)
        {
          m_GlobalReqFIFO.InsertElement(m_ReqBusMsg);
        }
      }
      else if (m_ReqBusMsg.cohrMsgId != SNOOPPrivCohTrans::UpgTrans)
      {
        m_GlobalReqFIFO.InsertElement(m_ReqBusMsg);
      }

      // advance TDM slot pointer
      m_reqCoreCnt = (m_reqCoreCnt == m_cpuCore - 1) ? 0 : (m_reqCoreCnt + 1);
      m_PndPutMChk = false;
    }

    // check pending request
    bool temp_PndReq = false;
    int pendingQueueSize = m_GlobalReqFIFO.GetQueueSize();
    BusIfFIFO::BusReqMsg txResp;

    // OOO TDM implementation
    for (int i = 0; i < m_cpuCore && m_PndReq == false; i++)
    {
      // check if the current candidate has a request in its local buffer
      temp_PndReq = CheckPendingReq(m_reqCoreCnt, m_ReqBusMsg, true);
      m_PndReq = temp_PndReq;

      // check if that core has a pending request in the service queue
      for (int j = 0; (j < pendingQueueSize && temp_PndReq == true && m_ReqBusMsg.cohrMsgId != SNOOPPrivCohTrans::PutMTrans); j++)
      {
        txResp = m_GlobalReqFIFO.GetFrontElement();
        m_GlobalReqFIFO.PopElement();
        if (m_reqCoreCnt == txResp.reqCoreId)
        {
          m_PndReq = false;
        }
        m_GlobalReqFIFO.InsertElement(txResp);
      }
      // advance current candidate if it doesn't have request
      if (!m_PndReq)
      {
        m_reqCoreCnt = (m_reqCoreCnt == m_cpuCore - 1) ? 0 : (m_reqCoreCnt + 1);
      }
      else
      { // fetch request message
        m_PndReq = CheckPendingReq(m_reqCoreCnt, m_ReqBusMsg, false);
      }
    }

    if (m_PndReq)
    {
      // wait one TDM Request slot
      Simulator::Schedule(NanoSeconds(m_dt * m_reqclks / 2), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
    }
    else
    {
      // wait one clock cycle and check again
      Simulator::Schedule(NanoSeconds(m_dt), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
    }
  } // void BusArbiter::PMSI_OOO_TDM_ReqBus()

  void BusArbiter::PISCOT_OOO_MSI_TDM_ReqBus()
  {
    BusIfFIFO::BusReqMsg txResp;
    // insert message on the request bus (broadcast)
    if (m_PndReq)
    {
      m_PndReq = CheckPendingReq(m_reqCoreCnt, m_ReqBusMsg, false);
      m_PndReq = false;
      // insert message on request bus
      InsertOnReqBus(m_ReqBusMsg);
      // insert message into service queue
      if (m_ReqBusMsg.NoGetMResp == false)
      {
        m_GlobalReqFIFO.InsertElement(m_ReqBusMsg);
      }
      // advance TDM slot pointer
      m_reqCoreCnt = (m_reqCoreCnt == m_cpuCore - 1) ? 0 : (m_reqCoreCnt + 1);
    }
    // check pending request
    bool temp_PndReq = false;
    int pendingQueueSize = m_GlobalReqFIFO.GetQueueSize();

    // OOO TDM implementation
    for (int i = 0; i < m_cpuCore && m_PndReq == false; i++)
    {
      // check if the current candidate has a request in its local buffer
      temp_PndReq = CheckPendingReq(m_reqCoreCnt, m_ReqBusMsg, true);
      m_PndReq = temp_PndReq;

      // check if that core has a pending request in the service queue
      for (int j = 0; (j < pendingQueueSize && temp_PndReq == true); j++)
      {
        txResp = m_GlobalReqFIFO.GetFrontElement();
        m_GlobalReqFIFO.PopElement();
        if (m_reqCoreCnt == txResp.reqCoreId)
        {
          m_PndReq = false;
        }
        m_GlobalReqFIFO.InsertElement(txResp);
      }

      // advance current candidate if it doesn't have request
      if (!m_PndReq)
      {
        m_reqCoreCnt = (m_reqCoreCnt == m_cpuCore - 1) ? 0 : (m_reqCoreCnt + 1);
      }
    }
    // wait one TDM Request slot, if there is any request
    if (m_PndReq)
    {
      Simulator::Schedule(NanoSeconds(m_dt * m_reqclks), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
    }
    else
    { // wait one clock cycle and check again
      Simulator::Schedule(NanoSeconds(m_dt), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
    }
  } // void BusArbiter::PISCOT_OOO_MSI_TDM_ReqBus() {

  void BusArbiter::MSI_FcFsReqBus()
  {
    // insert message on the request bus
    if (m_PndReq)
    {
      m_PndReq = CheckPendingFCFSReq(m_ReqBusMsg, false);
      InsertOnReqBus(m_ReqBusMsg);
      m_PndPutMChk = true;
      m_PndReq = false;
    }
    // insert request message into service queue
    if (m_PndPutMChk)
    {
      int pendingQueueSize = m_GlobalReqFIFO.GetQueueSize();
      // Insert PutM message if it is due to eviction only
      if (m_ReqBusMsg.cohrMsgId == SNOOPPrivCohTrans::PutMTrans && m_ReqBusMsg.msgId == 0)
      {
        bool PutInsertionDone = false;
        BusIfFIFO::BusReqMsg txReq;
        // iterate over service queue
        for (int i = 0; i < pendingQueueSize; i++)
        {
          txReq = m_GlobalReqFIFO.GetFrontElement();
          m_GlobalReqFIFO.PopElement();
          if (PutInsertionDone == false && (txReq.cohrMsgId == SNOOPPrivCohTrans::GetSTrans || SNOOPPrivCohTrans::GetMTrans) &&
              ((txReq.addr >> (int)log2(m_cacheBlkSize)) == (m_ReqBusMsg.addr >> (int)log2(m_cacheBlkSize))))
          {
            if (m_logFileGenEnable)
            {
              std::cout << "Skip PutM insertion into global buffer, This should not happen " << txReq.reqCoreId << std::endl;
            }
            PutInsertionDone = true;
          }
          // dequeuing
          m_GlobalReqFIFO.InsertElement(txReq);
        }
        // insert PutM if it was due to eviction
        if (PutInsertionDone == false)
        {
          m_GlobalReqFIFO.InsertElement(m_ReqBusMsg);
        }
      }
      else if (m_ReqBusMsg.cohrMsgId != SNOOPPrivCohTrans::UpgTrans)
      { // insert other requests as normal
        if (m_ReqBusMsg.NoGetMResp == false)
        { // Upgrade from "E" to "M" in MESI or MOESI
          m_GlobalReqFIFO.InsertElement(m_ReqBusMsg);
        }
      }
      m_PndPutMChk = false;
    }
    m_PndReq = CheckPendingFCFSReq(m_ReqBusMsg);
    // wait one Req-TDM slot
    if (m_PndReq)
    {
      Simulator::Schedule(NanoSeconds(m_dt * m_reqclks), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
    }
    else
    { // wait one clock cycle and check again
      Simulator::Schedule(NanoSeconds(m_dt), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
    }
  } //void BusArbiter::MSI_FcFsReqBus()

  void BusArbiter::MSI_FcFsReqBus2()
  {
    // insert message on the request bus
    if (m_PndReq)
    {
      m_PndReq = CheckPendingFCFSReq(m_ReqBusMsg, false);
      InsertOnReqBus(m_ReqBusMsg);
      m_PndReq = false;
    }

    m_PndReq = CheckPendingFCFSReq(m_ReqBusMsg);
    // wait one Req-TDM slot
    if (m_PndReq)
    {
      Simulator::Schedule(NanoSeconds(m_dt * m_reqclks), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
    }
    else
    { // wait one clock cycle and check again
      Simulator::Schedule(NanoSeconds(m_dt), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));
    }
  } //void BusArbiter::MSI_FcFsReqBus2()

  void BusArbiter::MSI_FcFsRespBus()
  {
    if (m_PndResp)
    {
      m_PndResp = CheckPendingFCFSResp(m_PendResp, false);
      m_PndResp = false;
    }

    m_PndResp = CheckPendingFCFSResp(m_PendResp);
    if (m_PndResp)
    {
      Simulator::Schedule(NanoSeconds(m_dt * m_respclks), &BusArbiter::RespStep, Ptr<BusArbiter>(this));
    }
    else
    { // wait one clock cycle and check again
      Simulator::Schedule(NanoSeconds(m_dt), &BusArbiter::RespStep, Ptr<BusArbiter>(this));
    }
  } //void BusArbiter::MSI_FcFsRespBus()

  void BusArbiter::PMSI_FcFsRespBus()
  {
    if (m_PndResp || m_FcFsPndMemResp)
    {
      m_FcFsPndMemResp = (m_FcFsPndMemResp == true) ? FcFsMemCheckInsert(m_respCoreCnt, m_ServQueueMsg.addr, false) : FcFsWriteBackCheckInsert(m_respCoreCnt, m_ServQueueMsg.addr, false, m_PendResp);
      m_GlobalReqFIFO.PopElement();
      if (m_cach2Cache && m_PndResp && m_ServQueueMsg.msgId != 0)
      {
        m_GlobalReqFIFO.PopElement();
      }
    }
    m_PndResp = false;
    m_FcFsPndMemResp = false;
    if (!m_GlobalReqFIFO.IsEmpty())
    {
      m_ServQueueMsg = m_GlobalReqFIFO.GetFrontElement();
      if (m_ServQueueMsg.cohrMsgId == SNOOPPrivCohTrans::PutMTrans)
      {
        m_respCoreCnt = m_ServQueueMsg.wbCoreId;
        m_PndResp = FcFsWriteBackCheckInsert(m_respCoreCnt, m_ServQueueMsg.addr, true, m_PendResp);
      }
      else
      { // this is a memory transfer slot
        m_respCoreCnt = m_ServQueueMsg.reqCoreId;
        m_FcFsPndMemResp = FcFsMemCheckInsert(m_respCoreCnt, m_ServQueueMsg.addr);
      }
    }
    if (m_PndResp == true || m_FcFsPndMemResp == true)
    {
      Simulator::Schedule(NanoSeconds(m_dt * m_respclks), &BusArbiter::RespStep, Ptr<BusArbiter>(this));
    }
    else
    { // wait one clock cycle and check again
      Simulator::Schedule(NanoSeconds(m_dt), &BusArbiter::RespStep, Ptr<BusArbiter>(this));
    }
  } //void BusArbiter::PMSI_FcFsRespBus()

  void BusArbiter::PISCOT_MSI_FcFsResBus()
  {
    if (m_PndResp || m_FcFsPndMemResp)
    {
      m_FcFsPndMemResp = (m_FcFsPndMemResp == true) ? FcFsMemCheckInsert(m_respCoreCnt, m_ServQueueMsg.addr, false) : FcFsWriteBackCheckInsert(m_respCoreCnt, m_ServQueueMsg.addr, false, m_PendResp);
      if ((m_cach2Cache == false && ((m_PndResp == false) || (m_ServQueueMsg.cohrMsgId == SNOOPPrivCohTrans::PutMTrans))) || (m_cach2Cache == true && ((m_PndResp == false) || (m_PndResp == true && m_PendResp.reqCoreId != m_sharedCacheBusIfFIFO->m_interface_id) || (m_ServQueueMsg.cohrMsgId == SNOOPPrivCohTrans::PutMTrans))))
      {
        m_GlobalReqFIFO.PopElement();
      }
    }

    m_PndResp = false;
    m_FcFsPndMemResp = false;
    // check service queue
    if (!m_GlobalReqFIFO.IsEmpty())
    {
      m_ServQueueMsg = m_GlobalReqFIFO.GetFrontElement();

      if (m_ServQueueMsg.cohrMsgId == SNOOPPrivCohTrans::PutMTrans)
      {
        m_respCoreCnt = m_ServQueueMsg.wbCoreId;
        m_PndResp = FcFsWriteBackCheckInsert(m_respCoreCnt, m_ServQueueMsg.addr, true, m_PendResp);
        if (m_PndResp == false)
        {
          m_TimeOut = m_TimeOut + 1;
          if (m_TimeOut == 5)
          {
            m_TimeOut = 0;
            m_GlobalReqFIFO.PopElement();
          }
        }
        else
        {
          m_TimeOut = 0;
        }
      }
      else
      {
        m_respCoreCnt = m_ServQueueMsg.reqCoreId;
        m_FcFsPndMemResp = FcFsMemCheckInsert(m_respCoreCnt, m_ServQueueMsg.addr);

        if (m_FcFsPndMemResp == false)
        {
          for (int i = 0; i < m_cpuCore && m_PndResp == false; i++)
          {
            m_respCoreCnt = i;
            m_PndResp = FcFsWriteBackCheckInsert(m_respCoreCnt, m_ServQueueMsg.addr, true, m_PendResp);
          }
        }
      }
    }
    // wait Resp-TDM response slot
    if (m_PndResp == true || m_FcFsPndMemResp == true)
    {
      Simulator::Schedule(NanoSeconds(m_dt * m_respclks), &BusArbiter::RespStep, Ptr<BusArbiter>(this));
    }
    else
    { // wait one clock cycle and check again
      Simulator::Schedule(NanoSeconds(m_dt), &BusArbiter::RespStep, Ptr<BusArbiter>(this));
    }
  } //void BusArbiter::PISCOT_MSI_FcFsResBus()

  void BusArbiter::init()
  {
    BusArbDecode();
    Simulator::Schedule(NanoSeconds(m_clkSkew), &BusArbiter::ReqStep, Ptr<BusArbiter>(this));    //Request
    Simulator::Schedule(NanoSeconds(m_clkSkew), &BusArbiter::RespStep, Ptr<BusArbiter>(this));   //Response
    Simulator::Schedule(NanoSeconds(m_clkSkew), &BusArbiter::Step, Ptr<BusArbiter>(this));       //Watch long stall to end the execution
    Simulator::Schedule(NanoSeconds(m_clkSkew), &BusArbiter::L2CohrStep, Ptr<BusArbiter>(this)); //L2 messages
  }

  void BusArbiter::ReqFncCall()
  {
    if (m_bus_arb == BusARBType::UNIFIED_TDM_ARB)
    {
      if (m_cohProType == CohProtType::SNOOP_PMSI || m_cohProType == CohProtType::SNOOP_PMESI)
        TDM_PMSI();
      else
        TDM_PCC();
    }
    else if (m_bus_arb == BusARBType::UNIFIED_RR_ARB ||
             m_bus_arb == BusARBType::UNIFIED_WRR_ARB ||
             m_bus_arb == BusARBType::UNIFIED_HRR_ARB)
    {
      RR_PCC();
    }
    else if (m_bus_arb == BusARBType::UNIFIED_FCFS_ARB)
    {
      FCFS_PCC();
    }
    else if (m_bus_arb == BusARBType::PISCOT_ARB)
    {
      if (m_cohProType == CohProtType::SNOOP_MSI ||
          m_cohProType == CohProtType::SNOOP_MESI ||
          m_cohProType == CohProtType::SNOOP_MOESI)
      {
        PISCOT_OOO_MSI_TDM_ReqBus();
      }
      else if (m_cohProType == CohProtType::SNOOP_PMSI)
      {
        PMSI_OOO_TDM_ReqBus();
      }
      else
      {
        std::cout << "BusArbiter: Un-supported Coh. Protocol" << std::endl;
        exit(0);
      }
    }
    else if (m_bus_arb == BusARBType::FCFS_ARB)
    {
      if (m_cohProType == CohProtType::SNOOP_MSI || m_cohProType == CohProtType::SNOOP_PMSI || m_cohProType == CohProtType::SNOOP_MESI || m_cohProType == CohProtType::SNOOP_MOESI)
      {
        MSI_FcFsReqBus2();
      }
      else
      {
        std::cout << "BusArbiter: Un-supported Coh. Protocol" << std::endl;
        exit(0);
      }
    }
  }

  void BusArbiter::L2CohrMsgHandle()
  {
    BusIfFIFO::BusReqMsg txreqMsg;
    if (!m_sharedCacheBusIfFIFO->m_txMsgFIFO.IsEmpty())
    {
      txreqMsg = m_sharedCacheBusIfFIFO->m_txMsgFIFO.GetFrontElement();
      if (txreqMsg.cohrMsgId == SNOOPPrivCohTrans::ExclTrans)
      {
        SendMemCohrMsg(txreqMsg, false);
      }
      else
      {
        SendMemCohrMsg(txreqMsg, true);
      }
      m_sharedCacheBusIfFIFO->m_txMsgFIFO.PopElement();
    }
    Simulator::Schedule(NanoSeconds(m_dt / 4), &BusArbiter::L2CohrStep, Ptr<BusArbiter>(this));
  }

  void BusArbiter::RespFncCall()
  {
    if (m_bus_arb == BusARBType::UNIFIED_TDM_ARB)
    {
      std::cout << "BusArbiter: [Info] Unified TDM Bus response scheduling is handled using Unified_TDM_PMSI_Bus function call" << std::endl;
    }
    else if (m_bus_arb == BusARBType::PISCOT_ARB ||
             m_bus_arb == BusARBType::FCFS_ARB)
    {
      if (m_cohProType == CohProtType::SNOOP_MSI ||
          m_cohProType == CohProtType::SNOOP_MESI ||
          m_cohProType == CohProtType::SNOOP_MOESI)
      {
        if (m_bus_arb == BusARBType::PISCOT_ARB)
        {
          PISCOT_MSI_FcFsResBus();
        }
        else
        {
          MSI_FcFsRespBus();
        }
      }
      else if (m_cohProType == CohProtType::SNOOP_PMSI)
      {
        PMSI_FcFsRespBus();
      }
      else
      {
        std::cout << "BusArbiter: Un-supported Coh. Protocol" << std::endl;
        exit(0);
      }
    }
  }

  void BusArbiter::BusArbDecode()
  {
    if (m_bus_arch == "unified")
    {
      // if (m_cohProType != CohProtType::SNOOP_PMSI)
      // {
      //   exit(0);
      // }

      if (m_bus_arbiter == "PMSI" || (m_bus_arbiter == "CUSTOM" && m_reqbus_arb == "TDM" && m_respbus_arb == "TDM"))
      {
        m_bus_arb = BusARBType::UNIFIED_TDM_ARB;
      }
      else if (m_bus_arbiter == "CUSTOM" && m_reqbus_arb == "RR" && m_respbus_arb == "RR")
      {
        m_bus_arb = BusARBType::UNIFIED_RR_ARB;
      }
      else if (m_bus_arbiter == "CUSTOM" && m_reqbus_arb == "WRR" && m_respbus_arb == "WRR")
      {
        m_bus_arb = BusARBType::UNIFIED_WRR_ARB;
      }
      else if (m_bus_arbiter == "CUSTOM" && m_reqbus_arb == "HRR" && m_respbus_arb == "HRR")
      {
        m_bus_arb = BusARBType::UNIFIED_HRR_ARB;
      }
      else if (m_bus_arbiter == "CUSTOM" && m_reqbus_arb == "FCFS" && m_respbus_arb == "FCFS")
      {
        m_bus_arb = BusARBType::UNIFIED_FCFS_ARB;
      }
      else
      {
        exit(0);
      }

      if (m_cach2Cache == true && m_bus_arbiter == "PMSI")
      {
        exit(0);
      }
    }
    else if (m_bus_arch == "split")
    {
      if (m_bus_arbiter == "PISCOT")
      {
        m_bus_arb = BusARBType::PISCOT_ARB;
      }
      else if (m_bus_arbiter == "FCFS")
      {
        m_bus_arb = BusARBType::FCFS_ARB;
      }
      else
      {
        exit(0);
      }

      if (!(m_cohProType == CohProtType::SNOOP_PMSI || m_cohProType == CohProtType::SNOOP_MSI || m_cohProType == CohProtType::SNOOP_MESI || m_cohProType == CohProtType::SNOOP_MOESI))
      {
        exit(0);
      }
    }
  }

  /**
    * Call request/response bus functions
    * These function is called each interval dt
    */

  void BusArbiter::ReqStep(Ptr<BusArbiter> busArbiter)
  {
    busArbiter->ReqFncCall();
  }

  void BusArbiter::RespStep(Ptr<BusArbiter> busArbiter)
  {
    busArbiter->RespFncCall();
  }

  void BusArbiter::L2CohrStep(Ptr<BusArbiter> busArbiter)
  {
    busArbiter->L2CohrMsgHandle();
  }

  // Schedule the next run
  void BusArbiter::Step(Ptr<BusArbiter> busArbiter)
  {
    busArbiter->CycleAdvance();
  }

  void BusArbiter::CycleAdvance()
  {
    if (m_stallDetectionEnable)
    {
      m_stall_cnt = (m_PndReq) ? 0 : (m_stall_cnt + 1);

      if (m_stall_cnt >= Stall_CNT_LIMIT)
      {
        // exit(0);
      }
    }

    m_arbiCycle++;
    Simulator::Schedule(NanoSeconds(m_dt), &BusArbiter::Step, Ptr<BusArbiter>(this));
  }
}
