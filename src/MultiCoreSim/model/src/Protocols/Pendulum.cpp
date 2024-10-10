/*
 * File  :      Pendulum.cpp
 * Author:      Safin Bayes
 * Email :      bayess@mcmaster.ca
 *
 * Created On March 02, 2022
 */

#include "../../header/Protocols/Pendulum.h"
using namespace std;

namespace ns3
{
    Pendulum::Pendulum(GenericCache *cache, const string &fsm_path, int coreId, int sharedMemId) : MSIProtocol(cache, fsm_path, coreId, sharedMemId)
    {
    }

    Pendulum::~Pendulum()
    {
    }
   vector<ControllerAction> &Pendulum::handleAction(vector<int> &actions, Message &msg,
                                                        const GenericCache::CacheLineInfo &cache_line_info, int next_state)
    {
        this->controller_actions.clear();
        for (int action : actions)
        {
            ControllerAction controller_action;
            switch (static_cast<ActionId>(action))
            {
            case ActionId::Stall: //return request to processing queue
                controller_action.type = ControllerAction::Type::ADD_PENDING;
                controller_action.data = (void *)new Message;
                ((Message *)controller_action.data)->copy(msg);
                break;

            case ActionId::Hit: //remove request from pending and respond to cpu, update cache line
                controller_action.type = (msg.source == Message::Source::LOWER_INTERCONNECT)
                                             ? ControllerAction::Type::HIT_Action
                                             : ControllerAction::Type::REMOVE_PENDING;
                controller_action.data = (void *)new Message;
                ((Message *)controller_action.data)->copy(msg);
                break;

            case ActionId::GetS:
            case ActionId::GetM:
            case ActionId::Inv_GetM:
                //add request to pending requests
                controller_action.type = ControllerAction::Type::ADD_PENDING;
                controller_action.data = (void *)new Message;
                ((Message *)controller_action.data)->copy(msg);
                this->controller_actions.push_back(controller_action);

                //send Bus request, update cache line
                controller_action.type = ControllerAction::Type::SEND_BUS_MSG;
                controller_action.data = (void *)new Message{.msg_id = msg.msg_id,
                                                             .addr = msg.addr,
                                                             .complementary_value = (uint16_t)action,
                                                             .from = (uint16_t)this->m_core_id,
                                                             .to = (uint16_t)this->m_shared_memory_id};
                break;

            case ActionId::PutM:
                //send Bus request, update cache line
                controller_action.type = ControllerAction::Type::SEND_BUS_MSG;
                controller_action.data = (void *)new Message{.msg_id = msg.msg_id,
                                                             .addr = msg.addr,
                                                             .complementary_value = (uint16_t)action,   //Ask About Complementary value
                                                             .from = (uint16_t)this->m_shared_memory_id,
                                                             .to = (uint16_t)this->m_core_id};
                break;

            case ActionId::Data2Req:
            case ActionId::Data2Both:
                //Do writeback, update cache line
                controller_action.type = ControllerAction::Type::WRITE_BACK;
                controller_action.data = (void *)new Message{.msg_id = msg.msg_id,
                                                             .addr = msg.addr,
                                                             .complementary_value = (uint16_t)((action == (int)ActionId::Data2Both) ? 1 : 0),
                                                             .from = (uint16_t)msg.from,
                                                             .to = (uint16_t)this->m_core_id};
                break;

            case ActionId::SaveReq:
                //send Bus request, update cache line
                controller_action.type = ControllerAction::Type::SAVE_REQ_FOR_WRITE_BACK;
                controller_action.data = (void *)new Message{.msg_id = msg.msg_id,
                                                             .addr = msg.addr,
                                                             .from = (uint16_t)msg.from,
                                                             .to = (uint16_t)this->m_core_id};
                break;

            case ActionId::SelfInv:
                controller_action.type = ControllerAction::Type::SEND_BUS_MSG;
                controller_action.data = (void *)new Message{.msg_id = msg.msg_id,
                                                             .addr = msg.addr,
                                                             .complementary_value = (uint16_t)action,
                                                             .from = (uint16_t)this->m_core_id,
                                                             .to = (uint16_t)this->m_shared_memory_id};
                break;
            
            case ActionId::Stop_T:
            case ActionId::RT:
                controller_action.type = ControllerAction::Type::TIMER_ACTION;
                controller_action.data = (void *)new Message{.msg_id = msg.msg_id,
                                                             .addr = msg.addr,
                                                             .complementary_value = (uint16_t)((action == (int)ActionId::Stop_T) ? 1 : 0),
                                                             .from = (uint16_t)msg.from,
                                                             .to = (uint16_t)this->m_core_id};
                break;

            case ActionId::Fault:
                std::cout << " Pendulum: Fault Transaction detected" << std::endl;
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
                                        m_cache->CpuAddrMap(msg.addr).tag, msg.data);
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

    void Pendulum::readEvent(Message &msg, MSIProtocol::EventId *out_id)
    {
        switch (msg.source)
        {
        case Message::Source::SELF:
            *out_id = (MSIProtocol::EventId) EventId::Timeout;
            return;

        case Message::Source::UPPER_INTERCONNECT:
            if (msg.data != NULL)
            {
                *out_id = (MSIProtocol::EventId) EventId::Data;
            }
            else
            {
            switch (msg.complementary_value)
                {
                case SNOOPPrivCohTrans::InvGetMTrans:
                    *out_id = (msg.to == m_core_id) ? (MSIProtocol::EventId) EventId::Own_InvGetM : (MSIProtocol::EventId) EventId::Other_InvGetM;
                    return;
                case SNOOPPrivCohTrans::InvTrans:
                    *out_id = (MSIProtocol::EventId) EventId::Own_SelfInv;
                    return;
                default: 
                    break;
                }
            }
        default:
            break;
        }
        MSIProtocol::readEvent(msg, out_id);
    }

}