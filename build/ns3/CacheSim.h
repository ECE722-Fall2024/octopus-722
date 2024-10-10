/*
 * File  :      CacheSim.h
 * Author:      Mohammed Ismail
 * Email :      ismaim22@mcmaster.ca
 *
 * Created On October 4, 2022
 */

#ifndef _CacheSim_H
#define _CacheSim_H

#include "ns3/tinyxml.h"
#include "ns3/MCoreSimProjectXml.h"
#include "ns3/MCoreSimProject.h"

#include <thread>

namespace ns3
{
    class CacheSim
    {
    private:
        std::thread* simulator_thread;
        MCoreSimProject* project;

    public:
        CacheSim(const char *config_file_path, const char *output_logs_path);
        ~CacheSim();

        void run();
        void join();
    };
}

#endif /* _CacheSim_H */
