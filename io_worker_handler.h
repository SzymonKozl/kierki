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

using IOWorkerIntroCb = std::function<void(Side, int)>;
using IOWrokerTrickCb = std::function<void(int, Side, Card)>;
using IOWorkerRepeatOnTiemoutCb = std::function<void(int, SSendJob)>;

class IOWorkerHandler: public IOWorker {
public:
    IOWorkerHandler(
            int pipe_fd,
            int id,
            int sock_fd,
            IOWorkerExitCb exit_callback,
            IOWorkerPipeCloseCb pipe_close_callback,
            IOWorkerIntroCb intro_callback,
            IOWrokerTrickCb trick_callback,
            IOWorkerRepeatOnTiemoutCb repeat_callback,
            const net_address& clientAddr,
            const net_address& own_addr,
            Logger& logger,
            int timeout
            );
private:
    void pollAction() override;
    void quitAction() override;
    void timeoutAction() override;

    IOWrokerTrickCb trickCb;
    IOWorkerIntroCb introCb;
    IOWorkerRepeatOnTiemoutCb repeatCb;
    bool introduced;
};


#endif //KIERKI_IO_WORKER_HANDLER_H
