//
// Created by szymon on 12.05.24.
//

#ifndef KIERKI_IO_WORKER_HANDLER_H
#define KIERKI_IO_WORKER_HANDLER_H

#include "io_worker.h"
#include "../common/common_types.h"
#include "../common/card.h"
#include "../common/logger.h"

#include "functional"

using IOWorkerIntroCb = std::function<bool(Side, int)>;
using IOWrokerTrickCb = std::function<bool(int, Card, int)>;
using IOWorkerInvalidMsgCb = std::function<bool(int)>;

class IOWorkerHandler: public IOWorker {
public:
    IOWorkerHandler(
            int pipeFd,
            int id,
            int sockFd,
            IOWorkerExitCb exitCallback,
            IOWorkerTimeoutCb timeoutCallback,
            IOWorkerExecuteSafeCb execCallback,
            IOWorkerIntroCb introCallback,
            IOWrokerTrickCb trickCallback,
            IOWorkerInvalidMsgCb invalidCallback,
            const NetAddress& clientAddr,
            const NetAddress& ownAddr,
            int timeout,
            Logger& logger
            );
private:
    void socketAction() override;

    IOWrokerTrickCb trickCb;
    IOWorkerIntroCb introCb;
    IOWorkerInvalidMsgCb invalidCb;
    std::string nextIncoming;
};

#endif //KIERKI_IO_WORKER_HANDLER_H
