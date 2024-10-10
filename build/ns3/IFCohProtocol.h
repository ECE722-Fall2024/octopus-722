/*
 * File  :      IFCohProtocol.h
 * Author:      Salah Hessien
 * Email :      salahga@mcmaster.ca
 *
 * Created On July 14, 2020
 */


#ifndef _IFCohProtocol_H
#define _IFCohProtocol_H

#include "SNOOPProtocolCommon.h"

// ----------------------------------
// Coherence Protocol Interface Class
// ----------------------------------

namespace ns3 { 
  class IFCohProtocol {
    public:
     // private cache event processing
     virtual void SNOOPPrivEventProcessing 
          (SNOOPPrivEventType  eventType, 
           SNOOPPrivEventList  eventList,
           int                 &cacheState,       
           SNOOPPrivCohTrans   &trans2Issue,
           SNOOPPrivCtrlAction &ctrlAction,
           bool                cache2Cache
           ) {} 

     virtual SNOOPPrivEventPriority PrivCacheEventPriority 
          (SNOOPPrivEventList       eventList,
           SNOOPPrivEventsCacheInfo eventCacheInfoList
          ) {return SNOOPPrivEventPriority::WorkConserv;}

      // Check if block Is valid
      virtual bool IsValidBlk (int s){return false;}

      // Print State Names
      virtual std::string PrivStateName   (int s){return "I";}
     
     // shared cache event processing
     virtual void SNOOPSharedEventProcessing 
          (SNOOPSharedEventType                    eventType,
           bool                                    cache2Cache,
           SNOOPSharedEventList                    eventList,
           int                                     &cacheState, 
           SNOOPSharedOwnerState                   &ownerState,
           SNOOPSharedCtrlAction                   &ctrlAction
           ){}

      virtual int SNOOPSharedCacheWaitDRAMState () { return -1;}

      virtual std::string SharedStateName (int s){return "IorS";}

      virtual ~IFCohProtocol() { };
  };




}
#endif /* _IFCohProtocol_H */
