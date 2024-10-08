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
            NetAddress  sender,
            NetAddress  receiver,
            std::string  payload
            );

    std::string toString();
private:
    using time_pt_t = std::chrono::time_point<std::chrono::system_clock>;

    const time_pt_t registered;
    const NetAddress sender;
    const NetAddress receiver;
    const std::string payload;
};

#endif //KIERKI_MESSAGE_H
