//
// Created by szymon on 11.05.24.
//

#ifndef KIERKI_LOGGER_H
#define KIERKI_LOGGER_H

#include "message.h"

#include "string"
#include "mutex"


class Logger {
public:
    Logger(std::ostream& ostream);
    void log(Message m);
private:
    std::mutex mutex;
    std::ostream &output;
};


#endif //KIERKI_LOGGER_H
