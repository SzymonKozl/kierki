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
using IOWorkerDisconnectCb = std::function<void(Side, std::string, int, int)>;

class IOWorkerHandler: public IOWorker {
public:
    IOWorkerHandler(
            int pipe_fd,
            int id,
            int sock_fd,
            IOWorkerExitCb exit_callback,
            IOWorkerSysErrCb error_callback,
            IOWorkerIntroCb intro_callback,
            IOWrokerTrickCb trick_callback,
            IOWorkerDisconnectCb disconnect_callback,
            net_address client_addr,
            net_address own_addr
            );
private:
    void pollAction() override;
    void quitAction() override;

    Logger logger;
    IOWorkerDisconnectCb disconnectCb;
    IOWrokerTrickCb trickCb;
    IOWorkerIntroCb introCb;
    Side client_loc;
    bool introduced;
    net_address client_addr;
    net_address own_addr;
};


#endif //KIERKI_IO_WORKER_HANDLER_H
