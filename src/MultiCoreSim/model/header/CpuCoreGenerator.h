/*
 * File  :      MCoreSimProjectXml.h
 * Author:      Salah Hessien
 * Email :      salahga@mcmaster.ca
 *
 * Created On February 16, 2020
 */
#ifndef _CpuCoreGenerator_H
#define _CpuCoreGenerator_H

#include "ns3/ptr.h"
#include "ns3/object.h"
#include "ns3/core-module.h"
#include "IdGenerator.h"
#include "MemTemplate.h"
#include "Logger.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

namespace ns3 { 
  /**
   * brief CpuCoreGenerator Implements processor read/write activities
   * according to specific brenchmark, the messages are generated in a   
   * real time fashion according to the cycle value in benchmark file.
   * CpuCoreGenerator interface with Cache Controller through two buffers
   * The Req/Resp Buffers.
  */

  class CpuCoreGenerator : public ns3::Object {
  private:
    int         m_coreId;
    double      m_dt;      // CPU Clk period
    double      m_clkSkew; // CPU Clk Skew
    uint64_t    m_cpuCycle;

    uint64_t    m_cpuReqCnt;
    uint64_t    m_cpuRespCnt;

    uint64_t    m_prevReqFinishCycle,
                m_prevReqArriveCycle;

    bool        m_prevReqFinish;
   
    std::string m_bmFileName;
    std::string m_cpuTraceFileName;
    std::string m_CtrlsTraceFileName;

    CpuFIFO::ReqMsg  m_cpuMemReq;
    CpuFIFO::RespMsg m_cpuMemResp;
 
    // The bench-metric trace streams
    std::ifstream m_bmTrace;

    // The output stream for cpu ack tracing
    std::ofstream m_cpuTrace;

    // The output stream for internal ctrl(s) tracing
    std::ofstream m_ctrlsTrace;

     // A pointer to CPU Interface FIFO
     CpuFIFO* m_cpuFIFO;

    // Cpu request done flag
    bool m_cpuReqDone;

    // Cpu new request ready flag
    bool m_newSampleRdy;

    // Cpu Simulation Done Flag
    bool m_cpuCoreSimDone;

    // enable flag for LogFile
    bool m_logFileGenEnable;

    int m_number_of_OoO_requests;
    int m_sent_requests = 0;

    // Called by static method to process step
    // to insert new request or remove response
    // from assoicatedBuffers.
    void ProcessTxBuf();
    void ProcessRxBuf();

  public:
    // Override TypeId.
    static TypeId GetTypeId(void);

    // CPU constructor must associated with two buffers
    // one for Request channel and the ohter for Received
    // response.
    CpuCoreGenerator(CpuFIFO* associatedCpuFIFO);

    // Generator's destructor
    ~CpuCoreGenerator();

    // set benchmark file name
    void SetBmFileName (std::string bmFileName);

   // set cpu dump file name
    void SetCpuTraceFile (std::string fileName);

   // set internal state dump file name
    void SetCtrlsTraceFile (std::string fileName);

    // set CoreId
    void SetCoreId (int coreId);

    // get core id
    int GetCoreId ();

    // set dt
    void SetDt (double dt);

    // set clk skew
    void SetClkSkew (double clkSkew);

    // get dt
    int GetDt ();

    void SetLogFileGenEnable (bool logFileGenEnable);

    void SetOutOfOrderStages(int stages);

    // get simulation done flag
    bool GetCpuSimDoneFlag();

    // Initialize core generator
    void init();
  
    /**
     * Run CPUCoreGenerator every clock cycle to
     * insert or remove request/response, this function
     * does the scheduling
     */
     static void Step(Ptr<CpuCoreGenerator> cpuCoreGenerator);
  };

}

#endif /* _CpuCoreGenerator_H */
