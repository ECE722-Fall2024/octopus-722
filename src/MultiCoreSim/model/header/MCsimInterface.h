/*
 * File  :      MCsimInterface.h
 * Author:      Mohammed Ismail
 * Email :      ismaim22@mcmaster.ca
 *
 * Created On August 4, 2022
 */

#ifndef _MCsimInterface_H
#define _MCsimInterface_H

#include "ns3/ptr.h"
#include "ns3/object.h"
#include "ns3/core-module.h"

#include "CommunicationInterface.h"
#include "MCoreSimProjectXml.h"
#include "FRFCFS_Buffer.h"

#include "ns3/MCsim.h"

namespace ns3
{
    class MCsimInterface : public ns3::Object
    {
    protected:
        int m_id;
        vector<int>  m_llc_id;
        int m_llc_line_size;
        int m_llc_nbanks;

        double m_dt;
        double m_clk_skew;
        
        uint64_t m_clk_cycle;
        uint64_t m_read_count;
        uint64_t m_write_count;

        vector<Message> m_pending_requests;
        vector<Message> m_output_buffer;
        
        CommunicationInterface *m_lower_interface; // A pointer to the lower Interface FIFO

        FRFCFS_Buffer<Message, MCsimInterface> *m_processing_queue;
        
        MCsim::MultiChannelMemorySystem *m_mcsim;

        virtual void cycleProcess();
        virtual void processLogic();
        virtual void addRequests2ProcessingQueue(FRFCFS_Buffer<Message, MCsimInterface> &buf);

        virtual void read_callback(unsigned, uint64_t, uint64_t);
        virtual void write_callback(unsigned, uint64_t, uint64_t);

        inline uint64_t calculate_bank(uint64_t address)
        {
            return ((address >>((int)log2(m_llc_line_size)) )) & (m_llc_nbanks - 1));
        }

    public:
        static TypeId GetTypeId(void); // Override TypeId.

        MCsimInterface(MCoreSimProjectXml &projectXml, CommunicationInterface *lower_interface, vector<int>  llc_id);
        ~MCsimInterface();

        virtual void init();
        static void step(Ptr<MCsimInterface> memory_controller);

        virtual FRFCFS_State getRequestState(const Message &, FRFCFS_State);
    };
}

#endif /* _MCsimInterface_H */
