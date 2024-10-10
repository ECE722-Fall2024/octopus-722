/*
 * File  :      BusArbiter.h
 * Author:      Salah Hessien
 * Email :      salahga@mcmaster.ca
 *
 * Created On February 21, 2020
 */

#ifndef _BusArbiter_H
#define _BusArbiter_H

#include "ns3/ptr.h"
#include "ns3/object.h"
#include "ns3/core-module.h"
#include "MemTemplate.h"
#include "SNOOPProtocolCommon.h"
#include "Logger.h"
#include <string.h>
#include <vector>

//#define Stall_CNT_LIMIT 200000
#define Stall_CNT_LIMIT 20000
using namespace std;

namespace ns3 { 
  /**
   * brief BusArbiter manage the scheduling of the different core
   * message on the shared interconnect
  */
  
  enum BusARBType {
    PISCOT_ARB       = 0x000,  
    UNIFIED_TDM_ARB  = 0x100,  
    UNIFIED_RR_ARB  = 0x101,  
    UNIFIED_WRR_ARB  = 0x102,  
    UNIFIED_HRR_ARB  = 0x103,  
    UNIFIED_FCFS_ARB  = 0x104, 
    FCFS_ARB         = 0x200 
  };
  
  enum AGENT {
    CORE = 0,
    SHAREDMEM,
    INTERCONNECT
  };
  
  class BusArbiter : public ns3::Object {
  private:
    double   m_dt; 
    double   m_clkSkew; 
    uint16_t m_cpuCore;
    uint16_t m_reqclks;
    uint16_t m_respclks;
    uint32_t m_cacheBlkSize;
    bool     m_workconserv;
    uint16_t m_reqCoreCnt;
    uint16_t m_respCoreCnt;
    uint64_t m_arbiCycle;
    bool     m_logFileGenEnable;
    bool     m_cach2Cache;
    bool     m_PndReq,
             m_PndResp;
    int      m_TimeOut;
    bool     m_FcFsPndMemResp,
             m_PndPutMChk,
             m_DirectTransfer,
             m_IdleSlot,
             m_PndMemResp,
             m_PndWB;
    bool     m_ReqWbFlag[32];
   
          
    bool     m_stallDetectionEnable;
    uint64_t m_stall_cnt;

    // Bus Arbiteration Type
    BusARBType  m_bus_arb;
    
    string m_bus_arch;
    
    string m_bus_arbiter;
    
    string m_reqbus_arb;
    
    string m_respbus_arb;
    
    CohProtType m_cohProType;
    
    int         m_maxPendingReq;
    
    // request bus interface message
    BusIfFIFO::BusReqMsg m_ReqBusMsg;
    BusIfFIFO::BusReqMsg m_ServQueueMsg;
    BusIfFIFO::BusRespMsg m_PendWBMsg;
    BusIfFIFO::BusRespMsg m_PendResp;
    // service queue
    GenericFIFO <BusIfFIFO::BusReqMsg> m_GlobalReqFIFO;
    
    // A list of Cache Ctrl Bus interface buffers
    std::list<BusIfFIFO* > m_busIfFIFO;

    // A pointer to shared cache Bus IF buffers
    BusIfFIFO* m_sharedCacheBusIfFIFO;

     // A pointer to Inteconnect FIFOs
    ns3::Ptr<ns3::InterConnectFIFO>  m_interConnectFIFO;
    
    void BusArbDecode();
    
    void CycleAdvance ();
    
    void ReqFncCall ();
    
    void RespFncCall ();
    
    void L2CohrMsgHandle(); 
    
    void SendData (BusIfFIFO::BusRespMsg msg, AGENT agent);  
    
    void SendMemCohrMsg (BusIfFIFO::BusReqMsg msg, bool BroadCast);
       
    bool CheckPendingWB (uint16_t core_idx, BusIfFIFO::BusRespMsg & wbMsg, bool CheckOnly);

    bool CheckPendingPutM (BusIfFIFO::BusReqMsg reqMsg, BusIfFIFO::BusReqMsg & putmReq);
    
    bool CheckPendingInvPutM (uint64_t addr, BusIfFIFO::BusReqMsg & putmReq);    

    bool CheckPendingReq  (uint16_t core_idx, BusIfFIFO::BusReqMsg & txMsg, bool CheckOnly);

    bool InsertOnReqBus (BusIfFIFO::BusReqMsg txMsg);

    bool FcFsMemCheckInsert (uint16_t coreId,uint64_t addr, bool CheckOnly, bool SkipAddrCheck);

    bool FcFsWriteBackCheckInsert (uint16_t core_idx, uint64_t addr, bool CheckOnly, BusIfFIFO::BusRespMsg & txResp);

    void PMSI_TDM_ReqBus();

    void PMSI_OOO_TDM_ReqBus();

    void PMSI_FcFsRespBus();

    void PISCOT_MSI_TDM_ReqBus(); 

    void PISCOT_OOO_MSI_TDM_ReqBus();

    void MSI_FcFsReqBus();

    void PISCOT_MSI_FcFsResBus();

    bool CheckPendingFCFSReq (BusIfFIFO::BusReqMsg & txMsg, bool ChkOnly);
    
    bool CheckPendingFCFSResp (BusIfFIFO::BusRespMsg & txMsg, bool ChkOnly);
    
    void MSI_FcFsRespBus();
    
    void MSI_FcFsReqBus2();
    
    void Unified_TDM_PMSI_Bus ();
    
    void Unified_TDM_PMSI_Bus2 ();

  public:
    static TypeId GetTypeId(void);

    BusArbiter(std::list<BusIfFIFO* > associatedPrivCacheBusIfFIFO,
                         BusIfFIFO*   assoicateLLCBusIfFIFO,
                         ns3::Ptr<ns3::InterConnectFIFO>  interConnectFIFO);


    ~BusArbiter();

    void SetDt (double dt);

    int GetDt ();

    void SetClkSkew (double clkSkew);

    void SetCacheBlkSize (uint32_t cacheBlkSize);
    
    void SetNumPrivCore (int nPrivCores);

    void SetLogFileGenEnable (bool logFileGenEnable);

    void SetNumReqCycles (int ncycle);

    void SetNumRespCycles (int ncycle);
 
    void SetIsWorkConserv (bool workConservFlag);

    void SetCache2Cache (bool cach2Cache);
    
    void SetBusArchitecture (string bus_arch);
    
    void SetBusArbitration (string bus_arb);
    
    void SetReqBusArb (string reqbus_arb); 
    
    void SetRespBusArb (string respbus_arb);               
    
    void SetCohProtType (CohProtType ptype);
    
    void SetMaxPendingReq (int maxPendingReq);
   
    void init();
    
    bool CheckPendingMessage(uint16_t core_idx, BusIfFIFO::BusReqMsg &txMsg, bool &demanding_turn);
    bool getResponse(uint16_t coreId, BusIfFIFO::BusRespMsg *resp_msg);
    bool conductWriteback(uint16_t coreId, BusIfFIFO::BusRespMsg *resp_msg);
    void selectCore_TDM(uint64_t *selected_core, int clk_in_slot);
    void selectCore_RR(uint64_t *selected_core);
    void selectCore_FCFS(uint64_t *selected_core);
    void TDM_PCC();
    void RR_PCC();
    void FCFS_PCC();
    void TDM_PMSI();

    /**
     * Run Shared Bus instances every clock cycle to
     * update cache line states and generate coherence 
     * messages. This function does the scheduling
     */
     static void ReqStep(Ptr<BusArbiter> busArbiter);

     static void RespStep(Ptr<BusArbiter> busArbiter);

     static void L2CohrStep(Ptr<BusArbiter> busArbiter);
     
     static void Step(Ptr<BusArbiter> busArbiter);
  };
}

#endif /* _BusArbiter_H */
