//
// Created by szymon on 12.05.24.
//

#ifndef KIERKI_IO_WORKER_CONNECT_H
#define KIERKI_IO_WORKER_CONNECT_H

#include "io_worker.h"

#include "functional"


using IOWorkerConnectionMadeCb = std::function<void(int)>;

class IOWorkerConnect: IOWorker {
public:
    IOWorkerConnect(
            int pipe_fd,
            int id,
            int sock_fd,
            IOWorkerExitCb& exit_callback,
            IOWorkerSysErrCb& error_callback,
            IOWorkerConnectionMadeCb& accept_callback
            );
private:
    void pollAction() override;

    IOWorkerConnectionMadeCb accCb;
};


#endif //KIERKI_IO_WORKER_CONNECT_H
