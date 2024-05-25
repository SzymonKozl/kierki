//
// Created by szymon on 12.05.24.
//

#ifndef KIERKI_IO_WORKER_CONNECT_H
#define KIERKI_IO_WORKER_CONNECT_H

#include "io_worker.h"
#include "common_types.h"

#include "functional"


using IOWorkerConnectionMadeCb = std::function<void(int, net_address)>;

class IOWorkerConnect: public IOWorker {
public:
    IOWorkerConnect(
            int pipe_fd,
            int id,
            int sock_fd,
            IOWorkerExitCb exit_callback,
            IOWorkerPipeCloseCb pipe_close_callback,
            IOWorkerConnectionMadeCb accept_callback,
            Logger& logger,
            const net_address &ownAddr
            );
private:
    void pollAction() override;
    void quitAction() override;

    IOWorkerConnectionMadeCb accCb;
};


#endif //KIERKI_IO_WORKER_CONNECT_H
