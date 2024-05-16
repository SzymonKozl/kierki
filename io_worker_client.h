//
// Created by szymon on 16.05.24.
//

#ifndef KIERKI_IO_WORKER_CLIENT_H
#define KIERKI_IO_WORKER_CLIENT_H

#include "io_worker.h"

#include "functional"

using IOWorkerTrickClientCb = std::function<void(Table, )

class IOWorkerClient: IOWorker {
public:
    void run();

    void newJob(SendJob &job);

    void scheduleDeath();

    IOWorkerClient(
            int pipe_fd,
            int id,
            int sock_fd,
            IOWorkerExitCb& exit_callback,
            IOWorkerSysErrCb& error_callback
    );

    ~IOWorkerClient();

protected:
    void pollAction() override;
    void disconnectAction() override;

    int id;
    bool terminate;
    const int main_fd;
    const int pipe_fd;
    JobQueue jobQueue;
    IOWorkerExitCb& exitCb;
    IOWorkerSysErrCb& errCb;
};


#endif //KIERKI_IO_WORKER_CLIENT_H
