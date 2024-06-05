//
// Created by szymon on 12.05.24.
//

#ifndef KIERKI_IO_WORKER_HANDLER_H
#define KIERKI_IO_WORKER_HANDLER_H

#include "io_worker.h"
#include "common_types.h"
#include "card.h"
#include "logger.h"

#include "functional"

using IOWorkerIntroCb = std::function<bool(Side, int)>;
using IOWrokerTrickCb = std::function<bool(int, Card, int)>;
using IOWorkerInvalidMsgCb = std::function<bool(int)>;

class IOWorkerHandler: public IOWorker {
public:
    IOWorkerHandler(
            int pipe_fd,
            int id,
            int sock_fd,
            IOWorkerExitCb exit_callback,
            IOWorkerTimeoutCb timeout_callback,
            IOWorkerExecuteSafeCb exec_callback,
            IOWorkerIntroCb intro_callback,
            IOWrokerTrickCb trick_callback,
            IOWorkerInvalidMsgCb invalid_callback,
            const net_address& clientAddr,
            const net_address& own_addr,
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
