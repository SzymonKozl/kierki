//
// Created by szymon on 11.05.24.
//

#include "message.h"

#include "utils.h"

#include "iostream"
#include "ctime"
#include "sstream"
#include "iomanip"
#include "chrono"

Message::Message(NetAddress  sender, NetAddress  receiver, std::string payload):
        registered(std::chrono::system_clock::now()),
        sender(std::move(sender)),
        receiver(std::move(receiver)),
        payload(std::move(payload))
{}

std::string Message::toString() {
    std::stringstream output("");
    output << "[" << formatAddr(sender) << "," << formatAddr(receiver) << ",";
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(registered.time_since_epoch()) % 1000;
    time_t tmp = std::chrono::system_clock::to_time_t(registered);
    std::tm* tm_ptr = std::localtime(&tmp);
    output << std::put_time(tm_ptr, "%Y-%m-%d %H:%M:%S");
    output << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();
    output << "] " << payload << "\n";
    return output.str();
}
