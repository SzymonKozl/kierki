//
// Created by szymon on 11.05.24.
//

#include "logger.h"
#include "message.h"
#include "constants.h"

#include "mutex"
#include "iostream"

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

void Logger::logSysErr(const ErrInfo &info) {
    std::lock_guard<std::mutex> lock(mutex);
    auto [call, eno, type] = info;
    if (type == IO_ERR_SILENT) return;
    std::cerr << "System call " << call << " resulted in error!. Errno: " << eno << '\n';
}
