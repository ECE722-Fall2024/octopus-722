/*
 * File  :      ExternalCPU.h
 * Author:      Mohammed Ismail
 * Email :      ismaim22@mcmaster.ca
 *
 * Created On Sept 6, 2022
 */

#ifndef _ExternalCPU_H
#define _ExternalCPU_H

#include "ns3/ptr.h"
#include "ns3/object.h"
#include "ns3/core-module.h"

#include "CommunicationInterface.h"
#include "CacheXml.h"
#include "FRFCFS_Buffer.h"
#include "IdGenerator.h"

#include <map>

namespace ns3
{
    template <typename Param1T, typename Param2T, typename Param3T>
    class CallbackGeneral
    {
        public:
		    virtual void operator()(Param1T, Param2T, Param3T) = 0;
    };

    template <typename ClassT, typename Param1T, typename Param2T, typename Param3T>
    class CPUCallback : public CallbackGeneral<Param1T, Param2T, Param3T>
    {
    private:
        typedef void (ClassT::*PtrMember)(Param1T, Param2T, Param3T);
        ClassT *const object;
        const PtrMember member;

    public:
        CPUCallback(ClassT *const object, PtrMember member) : object(object), member(member)
        {
        }

        CPUCallback(const CPUCallback<ClassT, Param1T, Param2T, Param3T> &e) : object(e.object), member(e.member)
        {
        }

        void operator()(Param1T param1, Param2T param2, Param3T param3)
        {
            return (const_cast<ClassT *>(object)->*member)(param1, param2, param3);
        }
    };

    enum RequestType
    {
        READ = 0,
        WRITE = 1,
        SETUP_WRITE = 2
    };

    class ExternalCPU : public ns3::Object
    {
    private:
        static map<int, ExternalCPU*> ext_CPUs;

    protected:
        int m_id;

        double m_dt;
        double m_clk_skew;
        uint64_t m_clk_cycle;

        CommunicationInterface *m_upper_interface; // A pointer to the upper Interface FIFO

        FRFCFS_Buffer<Message, ExternalCPU> *m_processing_queue;
        
        CallbackGeneral<uint64_t, uint64_t, RequestType>* m_cpu_callback; //Address, Clock Cycle, Type

        virtual void cycleProcess();
        virtual void processLogic();
        virtual void addRequests2ProcessingQueue(FRFCFS_Buffer<Message, ExternalCPU> &buf);

    public:
        static TypeId GetTypeId(void); // Override TypeId.

        ExternalCPU(CacheXml &xml, CommunicationInterface *upper_interface);
        ~ExternalCPU();

        virtual void init();
        static void step(Ptr<ExternalCPU> ext_cpu);

        virtual FRFCFS_State getRequestState(const Message &, FRFCFS_State);

        void registerCPUCallback(CallbackGeneral<uint64_t, uint64_t, RequestType>* cpu_callback);
        void addRequest(uint64_t address, RequestType type);

        static map<int, ExternalCPU*>* getExtCPUs();
    };
}

#endif /* _ExternalCPU_H */
