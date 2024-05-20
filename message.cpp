//
// Created by szymon on 11.05.24.
//

#include "message.h"

#include <utility>
#include "common_types.h"
#include "utils.h"

#include "iostream"
#include "ctime"
#include "sstream"
#include "iomanip"

Message::Message(net_address  sender, net_address  receiver, std::string payload):
        registered(std::time(nullptr)),
        sender(std::move(sender)),
        receiver(std::move(receiver)),
        payload(std::move(payload))
{}

std::string Message::toString() {
    std::string res;
    std::time_t tt = registered;
    std::stringstream output("");
    output << "[" << formatAddr(sender) << "," << formatAddr(receiver) << ",";
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(tt);
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()) % 1000;
    std::tm* tm_ptr = std::localtime(&tt);
    output << std::put_time(tm_ptr, "%Y-%m-%d %H:%M:%S");
    output << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();
    output << "] " << payload << "\\r\\n\n";
    return output.str();
}
