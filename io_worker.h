//
// Created by szymon on 11.05.24.
//

#ifndef KIERKI_IO_WORKER_H
#define KIERKI_IO_WORKER_H

#include "job_queue.h"
#include "common_types.h"

#include "functional"
#include "string"

using IOWorkerExitCb = std::function<void(int)>;
using IOWorkerSysErrCb = std::function<void(std::string, int, int, Side)>;

class IOWorker {
public:
    void run();

    void newJob(SendJob &job);

    void scheduleDeath();

    IOWorker(
            int pipe_fd,
            int id,
            int sock_fd,
            IOWorkerExitCb& exit_callback,
            IOWorkerSysErrCb& error_callback
        );

    virtual ~IOWorker();

protected:
    virtual void pollAction();
    virtual void disconnectAction();

    int id;
    bool terminate;
    const int main_fd;
    const int pipe_fd;
    JobQueue jobQueue;
    IOWorkerExitCb& exitCb;
    IOWorkerSysErrCb& errCb;
};


#endif //KIERKI_IO_WORKER_H
