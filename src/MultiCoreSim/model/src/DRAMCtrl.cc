/*
 * File  :      DRAMCtrl.cc
 * Author:      Salah Hessien
 * Email :      salahga@mcmaster.ca
 *
 * Created On February 25, 2021
 */

#include "../header/DRAMCtrl.h"

namespace ns3 {
    TypeId DRAMCtrl::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::DRAMCtrl")
               .SetParent<Object > ();
        return tid;
    }

    DRAMCtrl::DRAMCtrl(DRAMIfFIFO* associatedDRAMBusIfFifo) {
      m_dramBusIfFIFO     = associatedDRAMBusIfFifo;
      m_logFileGenEnable  = false;
      m_dramLatcy         = 100;
      m_dramOutstandReq   = 16;
      m_dramModle         = "FIXEDLat";
      m_outstandingReqCnt = 0;
      m_cycleCnt          = 0;
      m_dt                = (1.0/1000000);  
      m_clkSkew           = 0;   
      m_ctrlId            = 200;
      m_dramProcReads     = 0;
      m_dramProcWrites    = 0;
    }

    DRAMCtrl::~DRAMCtrl() {
    }

    void DRAMCtrl::SetLogFileGenEnable (bool logFileGenEnable ) {
      m_logFileGenEnable = logFileGenEnable;
    }
    
    void DRAMCtrl::SetDramFxdLatcy (uint32_t dramLatcy ) {
      m_dramLatcy = dramLatcy;
    }
    
    void DRAMCtrl::SetDramModel (std::string dramModel ) {
      m_dramModle = dramModel;
    }
    
    void DRAMCtrl::SetDramOutstandReq (uint32_t dramOutstandReqs) {
      m_dramOutstandReq = dramOutstandReqs;
    }

    void DRAMCtrl::SetMemCtrlId (int ctrlId) {
      m_ctrlId = ctrlId;
    }

    void DRAMCtrl::SetClkSkew (double clkSkew) {
       m_clkSkew = clkSkew;
    }
    
    void DRAMCtrl::SetDt (double dt) {
      m_dt = dt;
    }
    
    bool DRAMCtrl::ChkBusRxReqEvent  (DRAMIfFIFO::DRAMReqMsg &  busReqMsg, bool chkOnly = true) {
       if (!m_dramBusIfFIFO->m_txReqFIFO.IsEmpty()) {
         busReqMsg = m_dramBusIfFIFO->m_txReqFIFO.GetFrontElement();
         if (chkOnly == false) {
           m_dramBusIfFIFO->m_txReqFIFO.PopElement();
         }
         return true;
       }
       return false;
    }
         
   
    void DRAMCtrl::DRAMCtrlMain() {
    
      DRAMIfFIFO::DRAMReqMsg  busReqMsg;
      DRAMIfFIFO::DRAMRespMsg busRespMsg;
      bool DRAMNewReq;
      
      DRAMNewReq = ChkBusRxReqEvent  (busReqMsg, true);
      
      if (DRAMNewReq && !m_txProcFIFO.IsFull()) {
        DRAMNewReq = ChkBusRxReqEvent  (busReqMsg, false);
        busReqMsg.dramFetchcycle = m_cycleCnt;
        m_txProcFIFO.InsertElement(busReqMsg);
      }
      
      if(!m_txProcFIFO.IsEmpty()) {
        busReqMsg = m_txProcFIFO.GetFrontElement();
        if (busReqMsg.dramFetchcycle + m_dramLatcy < m_cycleCnt) {
          m_txProcFIFO.PopElement();
          
          if (busReqMsg.type == DRAMIfFIFO::DRAM_REQ::DRAM_READ) { // insert DRAM response
            busRespMsg.msgId     = busReqMsg.msgId;
            busRespMsg.addr      = busReqMsg.addr;
            busRespMsg.wbAgentId = busReqMsg.reqAgentId;
            busRespMsg.cycle     = m_cycleCnt;
            memcpy(busRespMsg.data, &busRespMsg.cycle, sizeof(busRespMsg.data)); //dummy data, cycle is merely chosen as it has the same size of the data
            m_dramBusIfFIFO->m_rxRespFIFO.InsertElement(busRespMsg);
            m_dramProcReads++;
          }
          else {
            m_dramProcWrites++;
          }      
        }
      } // if(!m_txProcFIFO.IsEmpty()) {
      
    } // void DRAMCtrl::DRAMCtrlMain() {
    
    void DRAMCtrl::CycleProcess() {
       DRAMCtrlMain();
      Simulator::Schedule(NanoSeconds(m_dt), &DRAMCtrl::Step, Ptr<DRAMCtrl > (this));
      m_cycleCnt++;
    }

    void DRAMCtrl::init() {
        m_txProcFIFO.SetFifoDepth (m_dramOutstandReq);
        Simulator::Schedule(NanoSeconds(m_clkSkew), &DRAMCtrl::Step, Ptr<DRAMCtrl > (this));
    }

    void DRAMCtrl::Step(Ptr<DRAMCtrl> dRAMCtrl) {
        dRAMCtrl->CycleProcess();
    }    
}
