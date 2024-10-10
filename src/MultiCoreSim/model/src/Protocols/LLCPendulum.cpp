/*
 * File  :      LLCPendulum.cpp
 * Author:      Safin Bayes
 * Email :      bayess@mcmaster.ca
 *
 * Created On March 07, 2022
 */

#include "../../header/Protocols/LLCPendulum.h"
using namespace std;

namespace ns3
{
    LLCPendulum::LLCPendulum(GenericCache *cache, const string &fsm_path, int coreId) : LLCMSIProtocol(cache, fsm_path, coreId)
    {
    }

    LLCPendulum::~LLCPendulum()
    {
    }


    vector<ControllerAction> &LLCPendulum::handleAction(vector<int> &actions, Message &msg,
                                                           const GenericCache::CacheLineInfo &cache_line_info, int next_state)
    {
        int line_owner_id = cache_line_info.owner_id;
        this->controller_actions.clear();

        for (int action : actions)
        {
            ControllerAction controller_action;
            switch (static_cast<ActionId>(action))
            {
            case ActionId::Stall: //return request to processing queue
                controller_action.type = ControllerAction::Type::ADD_PENDING;
                controller_action.data = (void *)new Message;
                ((Message*)controller_action.data)->copy(msg);
                break;
            
            case ActionId::IncrementSharer: //remove request from pending and respond to request
                controller_action.type = ControllerAction::Type::ADD_SHARER;
                controller_action.data = (void *)new Message;
                ((Message*)controller_action.data)->copy(msg);
                ((Message*)controller_action.data)->complementary_value = 0; //DualTrans == false
                break;

            case ActionId::DecrementSharer: //remove request from pending and respond to request
                controller_action.type = ControllerAction::Type::REMOVE_SHARER;
                controller_action.data = (void *)new Message;
                ((Message*)controller_action.data)->copy(msg);
                ((Message*)controller_action.data)->complementary_value = 0; //DualTrans == false
                break;    

            case ActionId::SendData: //remove request from pending and respond to request
                controller_action.type = ControllerAction::Type::REMOVE_PENDING;
                controller_action.data = (void *)new Message;
                ((Message*)controller_action.data)->copy(msg);
                ((Message*)controller_action.data)->complementary_value = 0; //DualTrans == false
                break;

            case ActionId::SaveReq:
                //send Bus request, update cache line
                controller_action.type = ControllerAction::Type::SAVE_REQ_FOR_WRITE_BACK;
                controller_action.data = (void *)new Message{.msg_id = msg.msg_id,
                                                             .addr = msg.addr,
                                                             .from = (uint16_t)msg.from,
                                                             .to = (uint16_t)this->m_core_id};

            case ActionId::SetOwner:
                line_owner_id = (msg.data == NULL) ? msg.from : msg.to;
                controller_action.type = ControllerAction::Type::NO_ACTION;
                break;

            case ActionId::ClearOwner:
                line_owner_id = -1;
                controller_action.type = ControllerAction::Type::NO_ACTION;
                break;

            case ActionId::SaveData: //update cacheline (it happens by default if Action is not Stall)
                controller_action.type = ControllerAction::Type::NO_ACTION;
                break;

            case ActionId::Fault:
                std::cout << " LLCPendulum Fault Transaction detected" << std::endl;
                exit(0);
                break;
            }

            this->controller_actions.push_back(controller_action);
        }

        //update cache line
        if ((actions.empty() && cache_line_info.IsExist) ||
            (!actions.empty() && actions[0] != (int)ActionId::Stall))
        {
            GenericCacheLine cache_line(next_state, this->m_fsm->isValidState(next_state),
                                        m_cache->CpuAddrMap(msg.addr).tag, msg.data, line_owner_id);
            delete[] msg.data;

            ControllerAction controller_action;
            controller_action.type = ControllerAction::Type::UPDATE_CACHE_LINE;
            controller_action.data = (void *)new uint8_t[sizeof(cache_line) + sizeof(cache_line_info.set_idx) + sizeof(cache_line_info.way_idx)];
            memcpy(controller_action.data, &cache_line, sizeof(cache_line));
            memcpy((uint8_t *)controller_action.data + sizeof(cache_line), &cache_line_info.set_idx, sizeof(cache_line_info.set_idx));
            memcpy((uint8_t *)controller_action.data + sizeof(cache_line) + sizeof(cache_line_info.set_idx),
                   &cache_line_info.way_idx, sizeof(cache_line_info.way_idx));

            this->controller_actions.push_back(controller_action);
        }

        return this->controller_actions;
    }
    
    void LLCPendulum::readEvent(Message &msg, GenericCache::CacheLineInfo cache_line_info, LLCMSIProtocol::EventId *out_id)
    {
        switch (msg.source)
        {
        case Message::Source::LOWER_INTERCONNECT:
            if (msg.data == NULL)
            { 
                switch (msg.complementary_value)
                {
                case SNOOPPrivCohTrans::InvGetMTrans:
                    *out_id = (LLCMSIProtocol::EventId) EventId::Inv_GetM;
                    return;
                case SNOOPPrivCohTrans::InvTrans:
                    *out_id = (LLCMSIProtocol::EventId) EventId::SelfInv;
                    return;
                default: // Invalid Transaction
                    break;
                }
            }
            break;

        case Message::Source::SELF:
            *out_id = (LLCMSIProtocol::EventId) EventId::Sharers_0;
            return;
        
        default:
            break;
        }

        LLCMSIProtocol::readEvent(msg, cache_line_info, out_id);
    }
}