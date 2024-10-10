/*
 * File  :      CommunicationInterface.h
 * Author:      Mohammed Ismail
 * Email :      ismaim22@mcmaster.ca
 *
 * Created On July 16, 2021
 */

#ifndef _COMMUNICATIONINTERFACE_H
#define _COMMUNICATIONINTERFACE_H

#include <stdint.h>
#include <string.h>
#include <vector>

enum MessageType
{
    REQUEST,
    DATA_RESPONSE,
    SERVICE_REQUEST,
};

class Message
{
public:
    uint64_t msg_id = 0; // Check if this can be removed and replaced by the addr
    uint64_t addr = 0;
    uint64_t cycle = 0;
    uint64_t complementary_value = 0;
    uint16_t owner = 0;

    std::vector<uint16_t> to;

    enum Source
    {
        LOWER_INTERCONNECT = 0,
        UPPER_INTERCONNECT,
        SELF
    } source;

    uint8_t *data = NULL;

    Message(uint64_t msg_id = 0, uint64_t addr = 0, uint64_t cycle = 0, uint64_t complementary_value = 0, uint16_t owner = 0)
    {
        this->msg_id = msg_id;
        this->addr = addr;
        this->cycle = cycle;
        this->complementary_value = complementary_value;
        this->owner = owner;

        this->source = LOWER_INTERCONNECT;
        this->to.clear();

        data = NULL;
    }

    Message(const Message &M2)
    {
        this->copy(M2);
    }

    ~Message()
    {
        if (this->data != NULL)
            delete[] this->data;
    }

    void copy(const Message &M2)
    {
        msg_id = M2.msg_id;
        addr = M2.addr;
        cycle = M2.cycle;
        complementary_value = M2.complementary_value;
        owner = M2.owner;
        source = M2.source;
        to = M2.to;

        if (M2.data != NULL)
            this->copy(M2.data);
        else
        {
            if (this->data != NULL)
                delete[] this->data;
            this->data = NULL;
        }
    }

    void copy(uint8_t *data)
    {
        if (this->data != NULL)
            delete[] this->data;
        this->data = new uint8_t[8]; // TODO: constant 8 should be converted to cache line size

        memcpy(this->data, data, 8);
    }

    Message &operator=(const Message &M2)
    {
        // Guard self assignment
        if (this == &M2)
            return *this;

        this->copy(M2);
        return *this;
    }
};

class CommunicationInterface
{
public:
    const int m_interface_id;

    CommunicationInterface(int id) : m_interface_id(id) {}
    virtual ~CommunicationInterface() {}

    virtual bool peekMessage(Message *out_msg) = 0;
    virtual void popFrontMessage() = 0;
    virtual bool pushMessage(Message &msg, uint64_t cycle, MessageType type = MessageType::REQUEST) = 0;
    virtual bool pushMessage2RX(Message &msg, MessageType type = MessageType::REQUEST) { return false; }

    virtual bool rollback(uint64_t address, uint64_t mask, Message *out_msg) { return false; }
};

#endif