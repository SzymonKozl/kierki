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
#include "chrono"
#include "memory"
#include "queue"

using IOWorkerExitCb = std::function<int(ErrArr, int, size_t)>;
using IOWorkerPipeCloseCb = std::function<void(int)>;
using IOWorkerTimeoutCb = std::function<void(int)>;
using IOWorkerExecuteSafeCb = std::function<bool(std::function<void()>)>;

class IOWorker {
public:
    void run();

    void newJob(const SSendJob& job);

    void scheduleDeath();

    IOWorker(
            int pipe_fd,
            int id,
            int sock_fd,
            IOWorkerExitCb exit_callback,
            IOWorkerPipeCloseCb pipe_close_callback,
            IOWorkerTimeoutCb timeout_callback,
            IOWorkerExecuteSafeCb exec_callback,
            int mainSockErr,
            net_address ownAddr,
            net_address clientAddr,
            int timeout,
            Logger& logger
        );

    ~IOWorker();

protected:
    virtual void socketAction() = 0;

    void handlePipe();
    void handleQueue();

    int id;
    bool wantToToQuit;
    bool terminate;
    const int main_fd;
    const int pipe_fd;
    JobQueue jobQueue;
    IOWorkerExitCb exitCb;
    IOWorkerTimeoutCb timeoutCb;
    IOWorkerPipeCloseCb pipeCb;
    IOWorkerExecuteSafeCb execCb;
    ErrArr errs;
    int mainSockErr;
    bool closedFd;
    net_address ownAddr;
    net_address clientAddr;
    std::shared_ptr<std::chrono::time_point<std::chrono::system_clock>> responseTimeout;
    int timeout;
    int nextTimeout;
    std::queue<std::string> pendingIncoming;
    std::queue<SSendJob> pendingOutgoing;
    Logger& logger;
};

using SIOWorker = std::shared_ptr<IOWorker>;

#endif //KIERKI_IO_WORKER_H
