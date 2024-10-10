/*
 * File  :      SNOOPProtocolCommon.h
 * Author:      Salah Hessien
 * Email :      salahga@mcmaster.ca
 *
 * Created On May 29, 2020
 */

#ifndef _SNOOPProtocolCommon_H
#define _SNOOPProtocolCommon_H

#include "ns3/ptr.h"
#include "ns3/object.h"
#include "ns3/core-module.h"

#include "MemTemplate.h"
// #include "GenericCache.h"

namespace ns3
{
  struct ControllerAction
  {
      enum Type
      {
          REMOVE_PENDING = 0,
          HIT_Action,
          ADD_PENDING,
          SEND_BUS_MSG,
          WRITE_BACK,
          UPDATE_CACHE_LINE,
          WRITE_CACHE_LINE_DATA,
          SAVE_REQ_FOR_WRITE_BACK,
          NO_ACTION,
          REMOVE_SAVED_REQ,
          TIMER_ACTION,
          ADD_SHARER,
          REMOVE_SHARER,
          SEND_INV_MSG,
          STALL,
      } type;
      void* data;
  };
  
  // enum CacheField
  // {
  //   State = 0,
  //   Tag,
  //   Data,
  //   Line
  // };

  enum CohProtType
  {
    SNOOP_PMSI = 0x000,
    SNOOP_PMESI = 0x001,
    SNOOP_PMSI_ASTERISK = 0x002,
    SNOOP_PMESI_ASTERISK = 0x003,
    SNOOP_MSI = 0x100,
    SNOOP_MESI = 0x200,
    SNOOP_MOESI = 0x300,
    SNOOP_LLC_MSI = 0x400,
    SNOOP_LLC_MESI = 0x500,
    SNOOP_LLC_MOESI = 0x600,
    SNOOP_LLC_PMSI = 0x700,
    SNOOP_LLC_PMESI = 0x800,
    SNOOP_LLC_PMSI_ASTERISK = 0x900,
    SNOOP_LLC_PMESI_ASTERISK = 0xA00,
    SNOOP_PENDULUM = 0xB00,
        SNOOP_LLC_PENDULUM = 0xC00,
    L1_MSI = 0xD00,
    L2_MSI = 0xE00
  };

  // enum SNOOPPrivCohTrans
  // {
  //   GetSTrans = 0,
  //   GetMTrans,
  //   PutMTrans,
  //   PutSTrans,
  //   UpgTrans,
  //   ExclTrans,
  //   InvTrans,
  //   NullTrans,
  //   InvGetMTrans
  // };

  // enum class SNOOPPrivEventType
  // {
  //   Core = 0,
  //   ReqBus,
  //   RespBus,
  //   Null
  // };

  // enum class SNOOPPrivCoreEvent
  // {
  //   Load = 0,
  //   Store,
  //   Replacement,
  //   Null
  // };

  // enum class SNOOPPrivReqBusEvent
  // {
  //   OwnGetS = 0,
  //   OwnGetM,
  //   OwnPutM,
  //   OwnPutS,
  //   OwnUpg,
  //   OtherGetS,
  //   OtherPutS,
  //   OtherGetM,
  //   OtherPutM,
  //   OtherUpg,
  //   OwnExclTrans,
  //   OtherExclTrans,
  //   OwnInvTrans,
  //   Null
  // };

  // enum class SNOOPPrivRespBusEvent
  // {
  //   OwnDataResp = 0,
  //   OwnDataRespExclusive,
  //   OtherDataResp,
  //   Null
  // };

  // enum class SNOOPPrivCtrlAction
  // {
  //   Stall = 0,
  //   Hit,
  //   issueTrans,
  //   ReissueTrans,
  //   issueTransSaveWbId,
  //   WritBack,
  //   CopyThenHit,
  //   CopyThenHitWB,
  //   CopyThenHitSendCoreOnly,
  //   CopyThenHitSendMemOnly,
  //   CopyThenHitSendCoreMem,
  //   SaveWbCoreId,
  //   HitSendMemOnly,
  //   SendMemOnly,
  //   SendCoreOnly,
  //   SendCoreMem,
  //   Fault,
  //   NoAck,
  //   NullAck,
  //   ProcessedAck
  // };

  // struct SNOOPPrivEventList
  // {
  //   SNOOPPrivCoreEvent cpuReqEvent;
  //   SNOOPPrivReqBusEvent busReqEvent;
  //   SNOOPPrivRespBusEvent busRespEvent;
  // };

  // struct SNOOPPrivEventsCacheInfo
  // {
  //   GenericCache::CacheLineInfo cpuReqCacheLineInfo;
  //   GenericCache::CacheLineInfo busReqCacheLineInfo;
  //   GenericCache::CacheLineInfo busRespCacheLineInfo;
  // };

  // struct SNOOPPrivStateList
  // {
  //   int cpuEventState;
  //   int busReqEventState;
  //   int busRespEventState;
  // };

  // enum class SNOOPPrivEventPriority
  // {
  //   ReqBus,
  //   RespBus,
  //   WorkConserv,
  // };

  // struct SNOOPPrivMsgList
  // {
  //   CpuFIFO ::ReqMsg cpuReqMsg = {};
  //   BusIfFIFO::BusReqMsg busReqMsg = {};
  //   BusIfFIFO::BusRespMsg busRespMsg = {};
  // };

  // enum class SNOOPSharedReqBusEvent
  // {
  //   GetS = 0,
  //   GetM,
  //   Upg,
  //   OwnerPutM,
  //   OTherPutM,
  //   PutS,
  //   Null
  // };

  // enum class SNOOPSharedCtrlEvent
  // {
  //   DRAMGetM,
  //   DRAMGetS,
  //   VictCacheSwap,
  //   Replacement,
  //   Null
  // };

  // enum class SNOOPSharedRespBusEvent
  // {
  //   OWnDataResp = 0,
  //   OTherDataResp,
  //   DRAMDataResp,
  //   NUll
  // };

  // enum class SNOOPSharedEventType
  // {
  //   ReqBus = 0,
  //   RespBus,
  //   CacheCtrl
  // };

  // struct SNOOPSharedEventList
  // {
  //   SNOOPSharedCtrlEvent CtrlEvent;
  //   SNOOPSharedReqBusEvent busReqEvent;
  //   SNOOPSharedRespBusEvent busRespEvent;
  // };

  // struct SNOOPSharedEventsCacheInfo
  // {
  //   GenericCache::CacheLineInfo busReqCacheLineInfo;
  //   GenericCache::CacheLineInfo busRespCacheLineInfo;
  // };

  // struct SNOOPSharedStateList
  // {
  //   int CtrlEventState;
  //   int busReqEventState;
  //   int busRespEventState;
  // };

  // struct SNOOPSharedMsgList
  // {
  //   BusIfFIFO::BusReqMsg busReqMsg = {};
  //   BusIfFIFO::BusRespMsg busRespMsg = {};
  // };

  // enum class SNOOPSharedCtrlAction
  // {
  //   SharedNoAck = 0,
  //   SharedStall,
  //   SendData,
  //   SendDataExclusive,
  //   SendExclusiveResp,
  //   StoreData,
  //   StoreDataOnly,
  //   SaveReqCoreId,
  //   SendPendingData,
  //   SendVictimCache,
  //   CopyDRAMIntoCache,
  //   IssueDRAMWrite,
  //   IssueCoreInvDRAMWrite,
  //   IssueCoreInv,
  //   SendExclusiveRespStall,
  //   SWAPPING,
  //   SharedFault,
  //   SharedNullAck,
  //   SharedProcessedAck
  // };

  // enum class SNOOPSharedOwnerState
  // {
  //   SharedMem = 0,
  //   OtherCore,
  //   SkipSet
  // };

  // struct SNOOPSharedOwnerList
  // {
  //   SNOOPSharedOwnerState CtrlOwnerState;
  //   SNOOPSharedOwnerState busReqOwnerState;
  //   SNOOPSharedOwnerState busRespOwnerState;
  // };

  // struct SNOOPSharedActionList
  // {
  //   SNOOPSharedCtrlAction CtrlAction;
  //   SNOOPSharedCtrlAction busReqAction;
  //   SNOOPSharedCtrlAction busRespAction;
  // };

}

#endif /* _SNOOPProtocolCommon_H */
