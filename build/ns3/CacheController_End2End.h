/*
 * File  :      CacheController_End2End.h
 * Author:      Mohammed Ismail
 * Email :      ismaim22@mcmaster.ca
 *
 * Created On May 22, 2022
 */

#ifndef _CacheController_End2End_H
#define _CacheController_End2End_H

#include "CacheController.h"

namespace ns3
{
    class CacheController_End2End : public CacheController
    {
    protected:
        int m_owner_of_latest_data;

        virtual void addRequests2ProcessingQueue(FRFCFS_Buffer<Message, CoherenceProtocolHandler> &buf) override;
        
        virtual void callActionFunction(ControllerAction) override;

        virtual void sendBusRequest(void *) override;
        virtual void performWriteBack(void *) override;
        virtual void sendInvalidationMessage(void *);

    public:
        static TypeId GetTypeId(void); // Override TypeId.

        CacheController_End2End(CacheXml &cacheXml, string &fsm_path,
                                CommunicationInterface *upper_interface, CommunicationInterface *lower_interface,
                                bool cach2Cache, int sharedMemId, CohProtType pType, vector<int> *private_caches_id = NULL);
        ~CacheController_End2End();
    };
}

#endif /* _CacheController_End2End_H */
