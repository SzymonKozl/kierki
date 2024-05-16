//
// Created by szymon on 11.05.24.
//

#ifndef KIERKI_MESSAGE_H
#define KIERKI_MESSAGE_H

#include "common_types.h"

#include "string"
#include "chrono"


class Message {
public:
    Message(
            net_address const& sender,
            net_address const& receiver,
            std::string const& payload
            );
    std::string toString();
private:
    const std::time_t registered;
    const net_address sender;
    const net_address receiver;
    const std::string payload;
};


#endif //KIERKI_MESSAGE_H
