//
// Created by szymon on 12.05.24.
//

#ifndef KIERKI_IO_WORKER_HANDLER_H
#define KIERKI_IO_WORKER_HANDLER_H

#include "io_worker.h"
#include "common_types.h"
#include "card.h"

#include "functional"

using IOWorkerIntroCb = std::function<void(Side)>;
using IOWrokerTrickCb = std::function<void(int, Side, Card)>;
using IOWorkerDisconnectCb = std::function<void(Side)>;

class IOWorkerHandler: IOWorker {
public:
    IOWorkerHandler(
            int sock_fd,
            int id,
            int pipe_fd,
            IOWorkerExitCb& exit_callback,
            IOWorkerSysErrCb& error_callback,
            IOWorkerIntroCb& intro_callback,
            IOWrokerTrickCb& trick_callback,
            IOWorkerDisconnectCb& disconnect_callback
            );
private:
    void pollAction() override;
    void disconnectAction() override;

    IOWorkerDisconnectCb disconnectCb;
    IOWrokerTrickCb trickCb;
    IOWorkerIntroCb introCb;
    Side client_loc;
    bool introduced;
};


#endif //KIERKI_IO_WORKER_HANDLER_H
