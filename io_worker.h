//
// Created by szymon on 11.05.24.
//

#ifndef KIERKI_IO_WORKER_H
#define KIERKI_IO_WORKER_H

#include "job_queue.h"
#include "common_types.h"
#include "memory"

#include "functional"
#include "string"

using IOWorkerExitCb = std::function<void(int)>;
using IOWorkerSysErrCb = std::function<void(errInfo info)>;

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
            IOWorkerExitCb& exit_callback,
            IOWorkerSysErrCb& error_callback
        );

protected:
    virtual void pollAction() = 0;
    virtual void quitAction() = 0;
    void informAboutError();

    int id;
    bool terminate;
    const int main_fd;
    const int pipe_fd;
    JobQueue jobQueue;
    IOWorkerExitCb& exitCb;
    IOWorkerSysErrCb& errCb;
    std::string err;
    int err_type;
};

using SIOWorker = std::shared_ptr<IOWorker>;

#endif //KIERKI_IO_WORKER_H
