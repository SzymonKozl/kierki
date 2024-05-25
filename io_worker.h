//
// Created by szymon on 11.05.24.
//

#ifndef KIERKI_IO_WORKER_H
#define KIERKI_IO_WORKER_H

#include "job_queue.h"
#include "common_types.h"
#include "memory"
#include "logger.h"

#include "functional"
#include "string"

using IOWorkerExitCb = std::function<void(ErrArr, int, Side)>;
using IOWorkerPipeCloseCb = std::function<void(int)>;

class IOWorker {
public:
    void run();

    void newJob(SSendJob job);

    void scheduleDeath();

    void halt();

    void unhalt();

    IOWorker(
            int pipe_fd,
            int id,
            int sock_fd,
            IOWorkerExitCb exit_callback,
            IOWorkerPipeCloseCb pipe_close_callback,
            int mainSockErr,
            Side side,
            Logger& logger
        );

    ~IOWorker();

protected:
    virtual void pollAction() = 0;
    virtual void quitAction() = 0;
    void informAboutError();

    int id;
    bool terminate;
    const int main_fd;
    const int pipe_fd;
    JobQueue jobQueue;
    IOWorkerExitCb exitCb;
    IOWorkerPipeCloseCb pipeCb;
    ErrArr errs;
    int mainSockErr;
    Side side;
    bool closedFd;
    Logger& logger;
};

using SIOWorker = std::shared_ptr<IOWorker>;

#endif //KIERKI_IO_WORKER_H
