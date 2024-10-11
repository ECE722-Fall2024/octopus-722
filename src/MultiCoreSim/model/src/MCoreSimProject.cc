/*
 * File  :      MCoreSimProject.cc
 * Author:      Salah Hessien
 * Email :      salahga@mcmaster.ca
 *
 * Created On February 15, 2020
 */

#include "../header/MCoreSimProject.h"
#include "ns3/simulator.h"

using namespace std;
using namespace ns3;

/*
 * Create the MCoreSimProject for the supplied configuration data
 */

MCoreSimProject::MCoreSimProject(MCoreSimProjectXml projectXmlCfg)
{
  // Set the project xml
  m_projectXmlCfg = projectXmlCfg;

  // Get clock frequency
  m_dt = projectXmlCfg.GetBusClkInNanoSec();
  m_busCycle = 0;

  // Get Run Till Sim End Flag
  m_runTillSimEnd = projectXmlCfg.GetRunTillSimEnd();

  // Get Simulation time to run
  m_totalTimeInSeconds = (m_runTillSimEnd == true) ? std::numeric_limits<int>::max() : projectXmlCfg.GetTotalTimeInSeconds();

  // Enable Log File Generation
  m_logFileGenEnable = projectXmlCfg.GetLogFileGenEnable();


  setup1(projectXmlCfg);
  // setup2(projectXmlCfg);
}

MCoreSimProject::~MCoreSimProject()
{
  for (CpuFIFO *cpuFIFO_ptr : m_cpuFIFO)
  {
    delete cpuFIFO_ptr;
  }
  m_cpuFIFO.clear();

  // delete m_sharedCacheBusIfFIFO;
  // delete m_sharedCacheDRAMBusIfFIFO;
}

void MCoreSimProject::setup1(MCoreSimProjectXml projectXmlCfg)
{
  /* Setup 1:
  - Fixed Latency DRAM
  - Private L1
  - One (non-bankized) L2 as LLC*/

  // initialize Simulator components
  m_cpuCoreGens = list<Ptr<CpuCoreGenerator>>();
  m_cpuFIFO = list<CpuFIFO *>();
  m_cpuCacheCtrl = list<CacheController *>();

  // Get all cpu configurations from xml
  list<CacheXml> xmlPrivateCaches = projectXmlCfg.GetPrivateCaches();
  list<CacheXml> xmlSharedCaches;

  CacheXml xmlSharedCache = projectXmlCfg.GetSharedCache();
  xmlSharedCaches.push_back(xmlSharedCache);

  // Get L1Bus configurations
  L1BusCnfgXml L1BusCnfg = projectXmlCfg.GetL1BusCnfg();

  char path_array[256];
  getcwd (path_array, sizeof(path_array));

  string path(path_array);
  //int app_name_index = path.rfind(APP_NAME);
  
  //m_fsm_protocol_path = path.substr(0, app_name_index + sizeof(APP_NAME) - 1) + string("/Protocols_FSM/");
  //m_fsm_llc_protocol_path = path.substr(0, app_name_index + sizeof(APP_NAME) - 1) + string("/Protocols_FSM/");
  m_fsm_protocol_path = path+string("/Protocols_FSM/");
  m_fsm_llc_protocol_path = path+string("/Protocols_FSM/");
  cout <<"FSM path: " << m_fsm_protocol_path << endl;

  // Get Coherence protocol type
  GetCohrProtocolType();

  m_maxPendReq = 0;

  bus = new TripleBus(xmlPrivateCaches, xmlSharedCaches, 
    projectXmlCfg.GetBusFIFOSize(), L1BusCnfg.GetReqBusLatcy(), L1BusCnfg.GetRespBusLatcy());

  // iterate over each core
  for (list<CacheXml>::iterator it = xmlPrivateCaches.begin(); it != xmlPrivateCaches.end(); it++)
  {
    CacheXml PrivateCacheXml = *it;

    /*
     * instantiate cpu FIFOs
     */
    CpuFIFO *newCpuFIFO = new CpuFIFO(PrivateCacheXml.GetCacheId(), projectXmlCfg.GetCpuFIFOSize());
    m_cpuFIFO.push_back(newCpuFIFO);

    /*
     * instantiate cpu cores
     */
    Ptr<CpuCoreGenerator> newCpuCore = CreateObject<CpuCoreGenerator>(newCpuFIFO);
    stringstream bmTraceFile, cpuTraceFile, ctrlTraceFile;
    bmTraceFile << projectXmlCfg.GetBMsPath() << "/trace_C" << PrivateCacheXml.GetCacheId() << ".trc.shared";
    cpuTraceFile << projectXmlCfg.GetBMsPath() << "/" << projectXmlCfg.GetCpuTraceFile() << PrivateCacheXml.GetCacheId() << ".txt";
    ctrlTraceFile << projectXmlCfg.GetBMsPath() << "/" << projectXmlCfg.GetCohCtrlsTraceFile() << PrivateCacheXml.GetCacheId() << ".txt";
    double cpuClkPeriod = PrivateCacheXml.GetCpuClkNanoSec();
    double cpuClkSkew = cpuClkPeriod * PrivateCacheXml.GetCpuClkSkew() / 100.00;
    newCpuCore->SetCoreId(PrivateCacheXml.GetCacheId());
    newCpuCore->SetBmFileName(bmTraceFile.str());
    newCpuCore->SetCpuTraceFile(cpuTraceFile.str());
    newCpuCore->SetCtrlsTraceFile(ctrlTraceFile.str());
    newCpuCore->SetDt(cpuClkPeriod);
    newCpuCore->SetClkSkew(cpuClkSkew);
    newCpuCore->SetLogFileGenEnable(m_logFileGenEnable);
    newCpuCore->SetOutOfOrderStages(projectXmlCfg.GetOutOfOrderStages());
    m_cpuCoreGens.push_back(newCpuCore);

    bm_paths.push_back(bmTraceFile.str());

    CommunicationInterface* bus_interface = bus->getInterfaceFor(PrivateCacheXml.GetCacheId());

    /*
     * instantiate cache controllers
     */
    CacheController *newCacheCtrl;
    if (m_cohrProt == CohProtType::SNOOP_MESI || m_cohrProt == CohProtType::SNOOP_MOESI)
      newCacheCtrl = new CacheControllerExclusive(PrivateCacheXml, m_fsm_protocol_path, bus_interface, newCpuFIFO,
                                                  projectXmlCfg.GetCache2Cache(), xmlSharedCache.GetCacheId(), m_cohrProt);
    else
      newCacheCtrl = new CacheController(PrivateCacheXml, m_fsm_protocol_path, bus_interface, newCpuFIFO,
                                         projectXmlCfg.GetCache2Cache(), xmlSharedCache.GetCacheId(), m_cohrProt);

    m_cpuCacheCtrl.push_back(newCacheCtrl);

    if (m_maxPendReq < PrivateCacheXml.GetNPendReq())
    {
      m_maxPendReq = PrivateCacheXml.GetNPendReq();
    }
  }

  bus2 = new Bus(xmlSharedCaches, projectXmlCfg.GetDRAMId(), projectXmlCfg.GetBusFIFOSize(), bus->getLowerLevelIds());
  CommunicationInterface* LLC_bus_interface = bus->getInterfaceFor(xmlSharedCache.GetCacheId());
  CommunicationInterface* LLC_DRAM_interface = bus2->getInterfaceFor(xmlSharedCache.GetCacheId());

  m_SharedCacheCtrl = new CacheController_End2End(xmlSharedCache, m_fsm_llc_protocol_path, LLC_DRAM_interface, LLC_bus_interface,
                                          projectXmlCfg.GetCache2Cache(), projectXmlCfg.GetDRAMId(), m_llcCohrProt, bus->getLowerLevelIds());


  CommunicationInterface* DRAM_LLC_interface = bus2->getInterfaceFor(projectXmlCfg.GetDRAMId());
  m_main_memory = new MainMemoryController(projectXmlCfg, DRAM_LLC_interface, xmlSharedCache.GetCacheId());
  // m_mcsim_interface = new MCsimInterface(projectXmlCfg, DRAM_LLC_interface, xmlSharedCache.GetCacheId());

  Logger::getLogger()->registerReportPath(projectXmlCfg.GetBMsPath() + string("/newLogger"));   

}



void MCoreSimProject::setup2(MCoreSimProjectXml projectXmlCfg, bool use_mcsim)
{

   // Setup3 is the more generic
   // setting number of Shared Caches to 1 will lead to Setup2


  /* Setup 2:
  - Fixed Latency DRAM
  - Private L1
  - Single (non-bankized) L2 as LLC*/

  /* Setup 3:
  - Fixed Latency DRAM
  - Private L1
  - Bankized L2 as LLC*/

  // initialize Simulator components
  m_cpuCoreGens = list<Ptr<CpuCoreGenerator>>();
  m_cpuFIFO = list<CpuFIFO *>();
  m_cpuCacheCtrl = list<CacheController *>();

  // Get all cpu configurations from xml
 // Get all cpu configurations from xml
  list<CacheXml> xmlPrivateCaches = projectXmlCfg.GetL1Caches();
  list<CacheXml> xmlSharedCaches = projectXmlCfg.GetSharedCache();

  


  

  char path_array[256];
  getcwd (path_array, sizeof(path_array));

  string path(path_array);
  m_fsm_protocol_path = path+string("/Protocols_FSM/");
  m_fsm_llc_protocol_path = path+string("/Protocols_FSM/");
  cout <<"FSM path: " << m_fsm_protocol_path << endl;

  // Get Coherence protocol type
  GetCohrProtocolType();

  m_maxPendReq = 0;

  /* Interconnects Setup*/
  // L1s <--> LLC
  
  // LLC <--> DRAM
 
  /* CPUs and L1s Setup*/
  for (list<CacheXml>::iterator it = xmlPrivateCaches.begin(); it != xmlPrivateCaches.end(); it++)
  {
    CacheXml PrivateCacheXml = *it;

    /*
     * instantiate cpu FIFOs
     */
    CpuFIFO *newCpuFIFO = new CpuFIFO(PrivateCacheXml.GetCacheId(), projectXmlCfg.GetCpuFIFOSize());
    m_cpuFIFO.push_back(newCpuFIFO);

    /*
     * instantiate cpu cores
     */
    Ptr<CpuCoreGenerator> newCpuCore = CreateObject<CpuCoreGenerator>(newCpuFIFO);
    stringstream bmTraceFile, cpuTraceFile, ctrlTraceFile;
    bmTraceFile << projectXmlCfg.GetBMsPath() << "/trace_C" << PrivateCacheXml.GetCacheId() << ".trc.shared";
    cpuTraceFile << projectXmlCfg.GetBMsPath() << "/" << projectXmlCfg.GetCpuTraceFile() << PrivateCacheXml.GetCacheId() << ".txt";
    ctrlTraceFile << projectXmlCfg.GetBMsPath() << "/" << projectXmlCfg.GetCohCtrlsTraceFile() << PrivateCacheXml.GetCacheId() << ".txt";
    double cpuClkPeriod = PrivateCacheXml.GetCpuClkNanoSec();
    double cpuClkSkew = cpuClkPeriod * PrivateCacheXml.GetCpuClkSkew() / 100.00;
    newCpuCore->SetCoreId(PrivateCacheXml.GetCacheId());
    newCpuCore->SetBmFileName(bmTraceFile.str());
    newCpuCore->SetCpuTraceFile(cpuTraceFile.str());
    newCpuCore->SetCtrlsTraceFile(ctrlTraceFile.str());
    newCpuCore->SetDt(cpuClkPeriod);
    newCpuCore->SetClkSkew(cpuClkSkew);
    newCpuCore->SetLogFileGenEnable(m_logFileGenEnable);
    newCpuCore->SetOutOfOrderStages(projectXmlCfg.GetOutOfOrderStages());
    m_cpuCoreGens.push_back(newCpuCore);

    bm_paths.push_back(bmTraceFile.str());

    // L1s Interface towards LLC Interconnect
    
    /*
     * instantiate cache controllers
     */
    CacheController *newCacheCtrl;
    if (m_cohrProt == CohProtType::SNOOP_MESI || m_cohrProt == CohProtType::SNOOP_MOESI)
      newCacheCtrl = new CacheControllerExclusive(PrivateCacheXml, m_fsm_protocol_path, bus_interface, newCpuFIFO,
                                                  projectXmlCfg.GetCache2Cache(), xmlSharedCacheIDs, m_cohrProt);
    else
      newCacheCtrl = new CacheController(PrivateCacheXml, m_fsm_protocol_path, bus_interface, newCpuFIFO,
                                         projectXmlCfg.GetCache2Cache(), xmlSharedCacheIDs, m_cohrProt);

    m_cpuCacheCtrl.push_back(newCacheCtrl);

    if (m_maxPendReq < PrivateCacheXml.GetNPendReq())
    {
      m_maxPendReq = PrivateCacheXml.GetNPendReq();
    }
  }

 
  /* LLC Setup */
  // iterate over each LLC
  // this variable assigns bank number for each LLC
  
  for (list<CacheXml>::iterator it = xmlSharedCaches.begin(); it != xmlSharedCaches.end(); it++)
  {

    // LLC Interface towards L1s Interconnect
    
    // LLC Interface towards DRAM Bus
    
    // LLC Controller
   
  }


  /* DRAM Setup */
  // DRAM Interface to LLC Bus
  CommunicationInterface* DRAM_LLC_interface = bus2->getInterfaceFor(projectXmlCfg.GetDRAMId());

  // DRAM Controller
  m_main_memory = new MainMemoryController(projectXmlCfg, DRAM_LLC_interface, xmlSharedCacheIDs);


  Logger::getLogger()->registerReportPath(projectXmlCfg.GetBMsPath() + string("/newLogger"));   

}
void MCoreSimProject::setup4(MCoreSimProjectXml projectXmlCfg)
{

  /* Setup 4:
  - Fixed Latency DRAM
  - Private L1 and L2
  - Shared (Bankized or non-Bankized) LLC*/


 
 // initialize Simulator components
  m_cpuCoreGens = list<Ptr<CpuCoreGenerator>>();
  m_cpuFIFO = list<CpuFIFO *>();
  m_cpuCacheCtrl = list<CacheController *>();


  // Get all cpu configurations from xml
  list<CacheXml> xmlL1Caches = projectXmlCfg.GetL1Caches();
  list<CacheXml> xmlL2Caches = projectXmlCfg.GetL2Caches();
  list<CacheXml> xmlSharedCaches = projectXmlCfg.GetSharedCache();

  

  char path_array[256];
  getcwd (path_array, sizeof(path_array));

  string path(path_array);
  m_fsm_protocol_path = path+string("/Protocols_FSM/");
  string l1_fsm = m_fsm_protocol_path + "MSI_L1.csv";
  string l2_fsm = m_fsm_protocol_path + "MSI_L2.csv";
  m_fsm_llc_protocol_path = m_fsm_protocol_path + "MSI_LLC.csv";
  cout <<"FSM path: " << m_fsm_protocol_path << endl;

  // Get Coherence protocol type
  // GetCohrProtocolType();

  // Interconnect between L2s and LLC
 
  // Bus between LLC and DRAM
  

  // DRAM interface towards LLC
  
  // Fixed Latency DRAM Controller
 
 /* LLC Setup */
  // iterate over each LLC
  for (list<CacheXml>::iterator it = xmlSharedCaches.begin(); it != xmlSharedCaches.end(); it++)
  {

    // LLC Interface towards L1s Interconnect
    
    // LLC Interface towards DRAM Bus
    CommunicationInterface* LLC_DRAM_interface = bus2->getInterfaceFor(it->GetCacheId());
    // LLC Controller
    
  }

 


/* CPUs, L1s,  and L2s Setup*/
  int core_num = 0;
  for (list<CacheXml>::iterator L1 = xmlL1Caches.begin(), L2 = xmlL2Caches.begin(); L1 != xmlL1Caches.end() && core_num < projectXmlCfg.GetNumPrivCore(); L1++, L2++, core_num++)
  {    



     /*
     * instantiate cpu FIFOs
     */
    CpuFIFO *newCpuFIFO = new CpuFIFO(L1->GetCacheId(), projectXmlCfg.GetCpuFIFOSize());
    m_cpuFIFO.push_back(newCpuFIFO);

    /*
     * instantiate cpu cores
     */
    Ptr<CpuCoreGenerator> newCpuCore = CreateObject<CpuCoreGenerator>(newCpuFIFO);
    stringstream bmTraceFile, cpuTraceFile, ctrlTraceFile;
    bmTraceFile << projectXmlCfg.GetBMsPath() << "/trace_C" << L1->GetCacheId() << ".trc.shared";
    cpuTraceFile << projectXmlCfg.GetBMsPath() << "/" << projectXmlCfg.GetCpuTraceFile() <<L1->GetCacheId() << ".txt";
    ctrlTraceFile << projectXmlCfg.GetBMsPath() << "/" << projectXmlCfg.GetCohCtrlsTraceFile() << L1->GetCacheId() << ".txt";
    double cpuClkPeriod = L1->GetCpuClkNanoSec();
    double cpuClkSkew = cpuClkPeriod * L1->GetCpuClkSkew() / 100.00;
    newCpuCore->SetCoreId(L1->GetCacheId());
    newCpuCore->SetBmFileName(bmTraceFile.str());
    newCpuCore->SetCpuTraceFile(cpuTraceFile.str());
    newCpuCore->SetCtrlsTraceFile(ctrlTraceFile.str());
    newCpuCore->SetDt(cpuClkPeriod);
    newCpuCore->SetClkSkew(cpuClkSkew);
    newCpuCore->SetLogFileGenEnable(m_logFileGenEnable);
    newCpuCore->SetOutOfOrderStages(projectXmlCfg.GetOutOfOrderStages());
    m_cpuCoreGens.push_back(newCpuCore);

    bm_paths.push_back(bmTraceFile.str());

  
    
    

   
    


  Logger::getLogger()->registerReportPath(projectXmlCfg.GetBMsPath() + string("/newLogger"));   
  // if (L1BusCnfg.GetReqBusArb() == "RR" ||
  //     L1BusCnfg.GetReqBusArb() == "WRR" ||
  //     L1BusCnfg.GetReqBusArb() == "HRR")
  //   Logger::getLogger()->setReplacementCorrection(L1BusCnfg.GetRespBusLatcy());
  // else
  //   Logger::
/*
 * start simulation engines
 */
void MCoreSimProject::Start()
{

  for (list<Ptr<CpuCoreGenerator>>::iterator it = m_cpuCoreGens.begin(); it != m_cpuCoreGens.end(); it++)
  {
    (*it)->init();
  }


  for (list<ExternalCPU *>::iterator it = m_ext_cpu.begin(); it != m_ext_cpu.end(); it++)
  {
    (*it)->init();
  }

  for (list<CacheController *>::iterator it = m_cpuCacheCtrl.begin(); it != m_cpuCacheCtrl.end(); it++)
  {
    (*it)->init();
  }

  m_SharedCacheCtrl->init();
  // m_SharedCacheCtrl->initializeCacheData(bm_paths);

  // m_dramCtrl->init();
  m_main_memory->init();
  // m_mcsim_interface->init();

  // m_busArbiter->init();
  bus->init();
  bus2->init();

  Simulator::Schedule(Seconds(0.0), &Step, this);
  Simulator::Stop(MilliSeconds(m_totalTimeInSeconds));
}

void MCoreSimProject::Step(MCoreSimProject *project)
{
  project->CycleProcess();
}

void MCoreSimProject::CycleProcess()
{
  bool SimulationDoneFlag = true;

  for (list<Ptr<CpuCoreGenerator>>::iterator it = m_cpuCoreGens.begin(); it != m_cpuCoreGens.end(); it++)
  {
    SimulationDoneFlag &= (*it)->GetCpuSimDoneFlag();
  }

  if (SimulationDoneFlag == true && m_cpuCoreGens.size() > 0)
  {
    cout << "Current Simulation Done at Bus Clock Cycle # " << m_busCycle << endl;
    cerr << "End\n";
    // cout << "L2 Nmiss =  " << m_SharedCacheCtrl->GetShareCacheMisses() << endl;
    // cout << "L2 NReq =  " << m_SharedCacheCtrl->GetShareCacheNReqs() << endl;
    // cout << "L2 Miss Rate =  " << (m_SharedCacheCtrl->GetShareCacheMisses() / (float)m_SharedCacheCtrl->GetShareCacheNReqs()) * 100 << endl;
    exit(0);
  }

  // Schedule the next run
  Simulator::Schedule(NanoSeconds(m_dt), &MCoreSimProject::Step, this);
  m_busCycle++;
}

void MCoreSimProject::EnableDebugFlag(bool Enable)
{

  // for (list<Ptr<CacheController>>::iterator it = m_cpuCacheCtrl.begin(); it != m_cpuCacheCtrl.end(); it++)
  // {
  //   (*it)->SetLogFileGenEnable(Enable);
  // }

  // m_SharedCacheCtrl->SetLogFileGenEnable(Enable);
  // m_busArbiter->SetLogFileGenEnable(Enable);
}

void MCoreSimProject::GetCohrProtocolType()
{
  string cohType = m_projectXmlCfg.GetCohrProtType();
  if (cohType == "MSI")
  {
    m_cohrProt = CohProtType::SNOOP_MSI;
    m_llcCohrProt = CohProtType::SNOOP_LLC_MSI;
    m_fsm_protocol_path += "MSI_splitBus_snooping.csv";
    m_fsm_llc_protocol_path += "MSI_LLC.csv";
  }
  else if (cohType == "MESI")
  {
    m_cohrProt = CohProtType::SNOOP_MESI;
    m_llcCohrProt = CohProtType::SNOOP_LLC_MESI;
    m_fsm_protocol_path += "MESI_splitBus_snooping.csv";
    m_fsm_llc_protocol_path += "MESI_LLC.csv";
  }
  else if (cohType == "MOESI")
  {
    m_cohrProt = CohProtType::SNOOP_MOESI;
    m_llcCohrProt = CohProtType::SNOOP_LLC_MOESI;
    m_fsm_protocol_path += "MOESI_splitBus_snooping.csv";
    m_fsm_llc_protocol_path += "MOESI_LLC.csv";
  }
  else if (cohType == "PMSI")
  {
    m_cohrProt = CohProtType::SNOOP_PMSI;
    m_llcCohrProt = CohProtType::SNOOP_LLC_PMSI;
    m_fsm_protocol_path += "PMSI.csv";
    m_fsm_llc_protocol_path += "PMSI_LLC.csv";
  }
  else if (cohType == "PMESI")
  {
    m_cohrProt = CohProtType::SNOOP_PMESI;
    m_llcCohrProt = CohProtType::SNOOP_LLC_PMESI;
    m_fsm_protocol_path += "PMESI.csv";
    m_fsm_llc_protocol_path += "PMESI_LLC.csv";
  }
  else if (cohType == "PMSI_Asterisk")
  {
    m_cohrProt = CohProtType::SNOOP_PMSI_ASTERISK;
    m_llcCohrProt = CohProtType::SNOOP_LLC_PMSI_ASTERISK;
    m_fsm_protocol_path += "PMSI_asterisk.csv";
    m_fsm_llc_protocol_path += "PMSI_asterisk_LLC.csv";
  }
  else if (cohType == "PMESI_Asterisk")
  {
    m_cohrProt = CohProtType::SNOOP_PMESI_ASTERISK;
    m_llcCohrProt = CohProtType::SNOOP_LLC_PMESI_ASTERISK;
    m_fsm_protocol_path += "PMESI_asterisk.csv";
    m_fsm_llc_protocol_path += "PMESI_asterisk_LLC.csv";
  }
  else
  {
    std::cout << "Unsupported Coherence Protocol Cnfg Param = " << cohType << std::endl;
    exit(0);
  }
}
