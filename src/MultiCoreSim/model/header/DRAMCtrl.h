/*
 * File  :      DRAMCtrl.h
 * Author:      Salah Hessien
 * Email :      salahga@mcmaster.ca
 *
 * Created On February 25, 2021
 */

#ifndef _DRAMCtrl_H
#define _DRAMCtrl_H


#include "ns3/ptr.h"
#include "ns3/object.h"
#include "ns3/core-module.h"
#include "MemTemplate.h"


#include <string>

namespace ns3 { 
  /**
   * brief DRAMCtrl Implements DRAM Memory Controller protocol, it is
   * the main interface between LLC and DRAM, the LLC Shared Cache
   * controller communicates with DRAM Ctrl through FIFO bi-directional
   * channel. 
  */
  
  class DRAMCtrl : public ns3::Object {
  private:
     // A pointer to DRAM Bus Interface FIFO
     DRAMIfFIFO* m_dramBusIfFIFO;
     bool        m_logFileGenEnable;
     uint32_t    m_dramLatcy;
     uint32_t    m_dramOutstandReq;
     std::string m_dramModle;
     uint32_t    m_outstandingReqCnt;
     uint64_t    m_cycleCnt;
     double      m_dt;     
     double      m_clkSkew; 
     int         m_ctrlId;
     uint64_t    m_dramProcReads;
     uint64_t    m_dramProcWrites;
     
     void CycleProcess ();
     
     void DRAMCtrlMain ();
     
     bool ChkBusRxReqEvent  (DRAMIfFIFO::DRAMReqMsg &  busReqMsg, bool chkOnly);
     
     GenericFIFO <DRAMIfFIFO::DRAMReqMsg  > m_txProcFIFO;
     
  public:
    static TypeId GetTypeId(void);

    DRAMCtrl (DRAMIfFIFO* associatedDRAMBusIfFifo);

    ~DRAMCtrl();
    
    void SetMemCtrlId (int ctrlId);

    void SetDramFxdLatcy (uint32_t dramLatcy );
    
    void SetDramModel (std::string dramModel );
    
    void SetDramOutstandReq (uint32_t dramOutstandReqs);
    
    void SetLogFileGenEnable (bool logFileGenEnable);
    
    void SetDt (double dt);
    
    void SetClkSkew (double clkSkew);
    
    void init();

     static void Step(Ptr<DRAMCtrl> dramCtrl);
     
  };
  
}

#endif /* _DRAMCtrl_H */
