//
// Created by szymon on 11.05.24.
//

#include "logger.h"
#include "message.h"

#include "mutex"
#include "chrono"

Logger::Logger(std::ostream &ostream, bool dummy):
    dummy(dummy),
    mutex(),
    output(ostream)
{}

void Logger::log(Message m) {
    if (dummy) return;
    std::lock_guard<std::mutex> lock(mutex);
    output << m.toString();
}
