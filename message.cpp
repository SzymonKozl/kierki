//
// Created by szymon on 11.05.24.
//

#include "message.h"

#include "ctime"

Message::Message(const std::string &sender, const std::string &receiver, const std::string &payload):
        sender(sender),
        receiver(receiver),
        payload(payload),
        registered(std::time(nullptr))
{}

const std::string &Message::getPayload() const noexcept {
    return payload;
}

const std::time_t &Message::getRegistered() const noexcept {
    return registered;
}

const std::string &Message::getSender() const noexcept {
    return sender;
}

const std::string &Message::getReceiver() const noexcept {
    return receiver;
}
