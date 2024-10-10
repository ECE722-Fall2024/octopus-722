/*
 * File  :      L1BusCnfgXml.h
 * Author:      Salah Hessien
 * Email :      salahga@mcmaster.ca
 *
 * Created On February 5, 2021
 */

#ifndef _L1BusCnfgXml_H
#define _L1BusCnfgXml_H
#include "tinyxml.h"
#include <string.h>

using namespace std;

class L1BusCnfgXml {
private:
  int    m_busClkNanoSec;
  int    m_busClkSkew;
  string m_busArch;
  string m_busArb;
  string m_reqBusArb;
  string m_respBusArb;
  int    m_reqBusLat;
  int    m_respBusLat;
  int    m_wrkConserv;
public:

  int GetBusClkNanoSec () {
     return m_busClkNanoSec;
  }

  int GetBusClkSkew () {
     return m_busClkSkew;
  }
  
  string GetBusArchitecture () {
     return m_busArch;
  }
  
  string GetBusArbitration() {
     return m_busArb;
  }
  
  string GetReqBusArb() {
     return m_reqBusArb;
  }

  string GetRespBusArb() {
     return m_respBusArb;
  }

  int GetReqBusLatcy () {
     return m_reqBusLat;
  }

  int GetRespBusLatcy () {
     return m_respBusLat;
  }
  
  int GetWrkConservFlag () {
     return m_wrkConserv;
  }

  void LoadFromXml(TiXmlHandle root) {

     // default values
     m_busClkNanoSec = 100;
     m_busClkSkew    = 0;
     m_busArch       = "unified";
     m_busArb        = "PMSI";
     m_reqBusArb     = "TDM";
     m_respBusArb    = "FCFS";
     m_wrkConserv    = 0;
     m_reqBusLat     = 4;
     m_respBusLat    = 50;
     
     TiXmlElement* L1BusCnfgRootPtr = root.Element();
     L1BusCnfgRootPtr->QueryIntAttribute    ("busClkNanoSec"  , &m_busClkNanoSec );
     L1BusCnfgRootPtr->QueryIntAttribute    ("busClkSkew"     , &m_busClkSkew    );
     L1BusCnfgRootPtr->QueryStringAttribute ("BusArch"        , &m_busArch       );
     L1BusCnfgRootPtr->QueryStringAttribute ("BusArb"         , &m_busArb        );
     L1BusCnfgRootPtr->QueryStringAttribute ("ReqBusArb"      , &m_reqBusArb     );
     L1BusCnfgRootPtr->QueryStringAttribute ("RespBusArb"     , &m_respBusArb    );
     L1BusCnfgRootPtr->QueryIntAttribute    ("ReqBusLat"      , &m_reqBusLat     );
     L1BusCnfgRootPtr->QueryIntAttribute    ("RespBusLat"     , &m_respBusLat    );
     L1BusCnfgRootPtr->QueryIntAttribute    ("WrkConserv"     , &m_wrkConserv    );
  }

};

#endif /* _L1BusCnfgXml_H */
