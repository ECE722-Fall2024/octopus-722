/*
 * File  :      MCoreSimProjectXml.h
 * Author:      Salah Hessien
 * Email :      salahga@mcmaster.ca
 *
 * Created On February 16, 2020
 */

#include "../header/CpuCoreGenerator.h"

namespace ns3 {

    // override ns3 type
    TypeId CpuCoreGenerator::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::CpuCoreGenerator")
               .SetParent<Object > ();
        return tid;
    }

    // The only constructor
    CpuCoreGenerator::CpuCoreGenerator(CpuFIFO* associatedCpuFIFO) {
        // default
        m_coreId         = 1;
        m_cpuCycle       = 1;
        m_bmFileName     = "trace_C0.trc";
        m_dt             = 1;
        m_clkSkew        = 0;
        m_cpuMemReq      = CpuFIFO::ReqMsg();
        m_cpuMemResp     = CpuFIFO::RespMsg();
        m_cpuFIFO        = associatedCpuFIFO;
        m_cpuReqDone     = false;
        m_newSampleRdy   = false;
        m_cpuCoreSimDone = false;
        m_logFileGenEnable = false;
        m_prevReqFinish    = true;
        m_prevReqFinishCycle = 0;
        m_prevReqArriveCycle = 0;
        m_cpuReqCnt      = 0;
        m_cpuRespCnt     = 0;
        m_number_of_OoO_requests = 1;
    }

    // We don't do any dynamic allocations
    CpuCoreGenerator::~CpuCoreGenerator() {
    }

    // set Benchmark file name
    void CpuCoreGenerator::SetBmFileName (std::string bmFileName) {
        m_bmFileName = bmFileName;
    }

    void CpuCoreGenerator::SetCpuTraceFile (std::string fileName) {
        m_cpuTraceFileName = fileName; 
    }

    void CpuCoreGenerator::SetCtrlsTraceFile (std::string fileName) {
        m_CtrlsTraceFileName = fileName;
    }

    // set CoreId
    void CpuCoreGenerator::SetCoreId (int coreId) {
      m_coreId = coreId;
    }

    // get core id
    int CpuCoreGenerator::GetCoreId () {
      return m_coreId;
    }

    // set dt
    void CpuCoreGenerator::SetDt (double dt) {
      m_dt = dt;
    }

    // get dt
    int CpuCoreGenerator::GetDt () {
      return m_dt;
    }

    // set clk skew
    void CpuCoreGenerator::SetClkSkew (double clkSkew) {
       m_clkSkew = clkSkew;
    }

    // get simulation done flag
    bool CpuCoreGenerator::GetCpuSimDoneFlag() {
      return m_cpuCoreSimDone;
    }

    void CpuCoreGenerator::SetLogFileGenEnable (bool logFileGenEnable ) {
      m_logFileGenEnable = logFileGenEnable;
    }

    void CpuCoreGenerator::SetOutOfOrderStages(int stages)
    {
      m_number_of_OoO_requests = stages;
      //std::cout<<"stages="<<m_number_of_OoO_requests<<std::endl;
    }
    
    // The init function starts the generator calling once very m_dt NanoSeconds.
    void CpuCoreGenerator::init() {
        m_bmTrace.open(m_bmFileName.c_str());
        Simulator::Schedule(NanoSeconds(m_clkSkew), &CpuCoreGenerator::Step, Ptr<CpuCoreGenerator > (this));
    }

    // This function does most of the functionality.
    void CpuCoreGenerator::ProcessTxBuf() {
        std::string fline;
        uint64_t newArrivalCycle;

        Logger::getLogger()->setClkCount(this->m_coreId, this->m_cpuCycle);
        
        if ((m_cpuFIFO->m_txFIFO.IsFull() == false) &&    // insert CPU request into buffer when
            //(m_cpuCycle >= m_cpuMemReq.cycle      ) &&    // fifo isn't full and cpucycle larger 
            (m_cpuReqDone == false                ) //&&     // than or equal cpu issued cycle
           // (m_prevReqFinish == true )
           )      
        {   
           if(m_sent_requests < m_number_of_OoO_requests)
           {
            if (m_newSampleRdy == true) { // wait until reading from file
                newArrivalCycle  = m_prevReqFinishCycle + m_cpuMemReq.cycle - m_prevReqArriveCycle;

                if (m_cpuCycle >= newArrivalCycle) {
                  // reset all flag when new request inserted in the FIFO
                  m_newSampleRdy   = false;
                  m_prevReqFinish  = false;

                  m_cpuMemReq.fifoInserionCycle = m_cpuCycle;

                  m_cpuFIFO->m_txFIFO.InsertElement(m_cpuMemReq);
                  Logger::getLogger()->addRequest(this->m_coreId, m_cpuMemReq);
                  m_sent_requests++;
                  if (m_logFileGenEnable) {
                    std::cout << "Cpu " << m_coreId << " MemReq: ReqId = " << m_cpuMemReq.msgId << ", CpuRefCycle = " 
                              << m_cpuMemReq.cycle << ", CpuClkTic ==================================================== " <<  m_cpuCycle << std::endl;
                    std::cout << "\t\tMemAddr = " << m_cpuMemReq.addr << ", ReqType (0:Read, 1:Write) = " << m_cpuMemReq.type << ", CpuTxFIFO Size = " << m_cpuFIFO->m_txFIFO.GetQueueSize() << std::endl << std::endl;
                  }
                }
            }
           }

          if (m_newSampleRdy == false) {
            if (getline(m_bmTrace,fline)) {
             m_newSampleRdy    = true;
             size_t pos        = fline.find(" ");
             std::string s     = fline.substr(0, pos); 
             std::string dummy = fline.substr(pos+1, 1); 
             std::string type  = fline.substr(pos+3, 1);
             std::string cyc   = fline.substr(pos+5);

             // convert hex string address to decimal 
             m_cpuMemReq.addr = (uint64_t) strtol(s.c_str(), NULL, 16);

             // convert cycle from string to decimal, same value as the file
             m_cpuMemReq.cycle= (uint64_t) strtol(cyc.c_str(), NULL, 10);

             m_cpuMemReq.type = (type == "R") ? CpuFIFO::REQTYPE::READ : CpuFIFO::REQTYPE::WRITE;
             //m_cpuMemReq.type = CpuFIFO::REQTYPE::WRITE;
             Ptr<UniformRandomVariable> uRnd1;
             uRnd1 = CreateObject<UniformRandomVariable> ();
             uRnd1-> SetAttribute ("Min", DoubleValue (0));
             uRnd1-> SetAttribute ("Max", DoubleValue (100));
             //m_cpuMemReq.type = (uRnd1->GetValue() <= 50) ? CpuFIFO::REQTYPE::READ : CpuFIFO::REQTYPE::WRITE;

             // Generate unique Id for every cpu request (needed to avoid any 
             // collisions with other cores and to make debugging easier).
             m_cpuMemReq.msgId     = IdGenerator::nextReqId();
             m_cpuMemReq.reqCoreId = m_coreId;
             m_cpuReqCnt++;
            }
          } 
        }

        if (m_bmTrace.eof()) {
          m_bmTrace.close();
          m_cpuReqDone = true;
        }           
    } // void CpuCoreGenerator::ProcessTxBuf()

        void CpuCoreGenerator::ProcessRxBuf() {
        // process received buffer
        if (!m_cpuFIFO->m_rxFIFO.IsEmpty()) {
          m_cpuMemResp = m_cpuFIFO->m_rxFIFO.GetFrontElement();
          m_cpuFIFO->m_rxFIFO.PopElement();
          Logger::getLogger()->updateRequest(m_cpuMemResp.msgId, Logger::EntryId::CPU_RX_CHECKPOINT);
          m_sent_requests--;
          if(m_sent_requests < 0)
            std::cout << "error" << std::endl;
          if (m_logFileGenEnable) {
            std::cout << "Cpu " << m_coreId << " new response is received at cycle " << m_cpuCycle << std::endl;
          }
          m_prevReqFinish      = true;
          m_prevReqFinishCycle = m_cpuCycle;
          m_prevReqArriveCycle = m_cpuMemResp.reqcycle;
          m_cpuRespCnt++;
        }
 
        // schedule next run or finish simulation if processing end
        if (m_cpuReqDone == true && m_cpuRespCnt >= m_cpuReqCnt) {
          m_cpuCoreSimDone = true;
          Logger::getLogger()->traceEnd(this->m_coreId);
          std::cout << "Cpu " << m_coreId << " Simulation End @ processor cycle # " << m_cpuCycle << std::endl;
        }
        else {
          // Schedule the next run
          Simulator::Schedule(NanoSeconds(m_dt), &CpuCoreGenerator::Step, Ptr<CpuCoreGenerator > (this));
          m_cpuCycle++;
        }

    } // CpuCoreGenerator::ProcessRxBuf()

    /**
     * Runs one mobility Step for the given vehicle generator.
     * This function is called each interval dt
     */
    void CpuCoreGenerator::Step(Ptr<CpuCoreGenerator> cpuCoreGenerator) {
        cpuCoreGenerator->ProcessTxBuf();
        cpuCoreGenerator->ProcessRxBuf();
    }
}
