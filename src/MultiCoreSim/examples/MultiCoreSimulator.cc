/*
 * File  :      MultiCoreSim.cc
 * Author:      Salah Hessien
 * Email :      salahga@mcmaster.ca
 *
 * Created On February 15, 2020
 */

#include "ns3/core-module.h"
#include "tinyxml.h"
#include "MCoreSimProjectXml.h"
#include "MCoreSimProject.h"


using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("MultiCoreSimulator");

int main (int argc, char *argv[])
{

  // simulator output Trace Files
  string InterConnectTraceFile = "InterCTrace.csv";   // Dump bus activities 
  string CohCtrlsTraceFile     = "CohCtrlsTrace.csv"; // Dump Internal states of 
                                                      // CPU Mem Controllers

  // simulator input files
  string SimConfigFile = "test.cfg"; // Configuration Parameter File
  string BMsPath = "BMs/tests/"; // Benchmark TraceFiles Path


  // command line parser
  CommandLine cmd;

  // adding a call to our sim configurable parameters 
  cmd.AddValue("CfgFile", "simulator configuration file", SimConfigFile);
  cmd.AddValue("BMsPath", "benchmark trace file(s) path", BMsPath);
  cmd.AddValue("BusTraceFile", "trace file for bus transactions", InterConnectTraceFile);
  cmd.AddValue("CtrlsTraceFile", "trace file for coherence controlles", CohCtrlsTraceFile);

  // Add debug enable and TraceEnable 
  // TBD

  cmd.Parse (argc, argv);

  // read Xml Configuration File
  TiXmlDocument doc(SimConfigFile.c_str());
  doc.LoadFile();
  TiXmlHandle hDoc(&doc);
  TiXmlElement* root = hDoc.FirstChildElement().Element();
  TiXmlHandle hroot = TiXmlHandle(root);
  MCoreSimProjectXml xml;
  xml.LoadFromXml(hroot);

  // setup simulation environment
  MCoreSimProject project;

//  project.SetInCfgFile      (SimConfigFile        );
//  project.SetBMsPath        (BMsPath              );
  project.SetBusTraceFile   (InterConnectTraceFile);
  project.SetCtrlsTraceFile (CohCtrlsTraceFile    );


  // set simulation clock to one nano-Second
  // clock resolution is the smallest time value 
  // that can be respresented in our simulator
  Time::SetResolution (Time::NS); // MS, US, PS

  // lunch simulator
  project.Start();
  Simulator::Run();

  // clean up once done
  Simulator::Destroy();
  return 0;

}
