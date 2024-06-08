//
// Created by szymon on 12.05.24.
//

#ifndef KIERKI_IO_WORKER_CONNECT_H
#define KIERKI_IO_WORKER_CONNECT_H

#include "io_worker.h"
#include "../common/common_types.h"
#include "../common/logger.h"

#include "functional"

using IOWorkerConnectionMadeCb = std::function<void(int, NetAddress)>;

class IOWorkerConnect: public IOWorker {
public:
    IOWorkerConnect(
            int pipeFd,
            int id,
            int sockFd,
            IOWorkerExitCb exitCallback,
            IOWorkerTimeoutCb timeoutCallback,
            IOWorkerExecuteSafeCb execCallback,
            IOWorkerConnectionMadeCb acceptCallback,
            const NetAddress &ownAddr,
            Logger& logger
            );
private:
    void socketAction() override;

    IOWorkerConnectionMadeCb accCb;
};

#endif //KIERKI_IO_WORKER_CONNECT_H
