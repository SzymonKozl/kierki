//
// Created by szymon on 11.05.24.
//

#include "logger.h"
#include "message.h"

#include "mutex"
#include "ctime"
#include "chrono"
#include "iomanip"

Logger::Logger(std::ostream &ostream, bool dummy):
    output(ostream),
    dummy(dummy),
    mutex()
{}

void Logger::log(Message m) {
    if (dummy) return;
    std::lock_guard<std::mutex> lock(mutex);
    output << m.toString();
}
