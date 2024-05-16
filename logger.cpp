//
// Created by szymon on 11.05.24.
//

#include "logger.h"
#include "message.h"

#include "mutex"
#include "ctime"
#include "chrono"
#include "iomanip"

Logger::Logger(std::ostream &ostream):
    output(ostream),
    mutex()
{}


void Logger::log(Message m) {
    std::lock_guard<std::mutex> lock(mutex);
    std::time_t tt = m.getRegistered();
    output << "[" << m.getSender() << "," << m.getReceiver() << ",";
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(tt);
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()) % 1000;
    std::tm* tm_ptr = std::localtime(&tt);
    output << std::put_time(tm_ptr, "%Y-%m-%d %H:%M:%S");
    output << '.' << std::setfill('0') << std::setw(3) << milliseconds.count() << std::endl;
    output << "] " << m.getPayload() << "\\r\\n";
}
