//
// Created by szymon on 11.05.24.
//

#ifndef KIERKI_MESSAGE_H
#define KIERKI_MESSAGE_H

#include "string"
#include "chrono"


class Message {
public:
    Message(
            std::string const& sender,
            std::string const& receiver,
            std::string const& payload
            );
    const std::string& getSender() const noexcept;
    const std::string& getReceiver() const noexcept;
    const std::string& getPayload() const noexcept;
    const std::time_t& getRegistered() const noexcept;
private:
    const std::time_t registered;
    const std::string sender;
    const std::string receiver;
    const std::string payload;
};


#endif //KIERKI_MESSAGE_H
