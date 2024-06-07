//
// Created by szymon on 11.05.24.
//

#ifndef KIERKI_IO_WORKER_H
#define KIERKI_IO_WORKER_H

#include "job_queue.h"
#include "../common/common_types.h"
#include "memory"
#include "../common/logger.h"

#include "functional"
#include "string"
#include "chrono"
#include "memory"
#include "queue"
#include "sys/poll.h"

using IOWorkerExitCb = std::function<int(ErrArr, int, bool)>;
using IOWorkerTimeoutCb = std::function<void(int)>;
using IOWorkerExecuteSafeCb = std::function<bool(std::function<void()>)>;

class IOWorker {
public:
    IOWorker(
            int pipeFd,
            int id,
            int sockFd,
            IOWorkerExitCb exitCallback,
            IOWorkerTimeoutCb timeoutCallback,
            IOWorkerExecuteSafeCb execCallback,
            IOErrClass mainSockErr,
            NetAddress ownAddr,
            NetAddress clientAddr,
            int timeout,
            Logger& logger
    );

    ~IOWorker();

    void run();

    void newJob(const SSendJob& job);

    void scheduleDeath();
protected:
    using TimeVal = std::chrono::time_point<std::chrono::system_clock>;

    virtual void socketAction() = 0;

    void handlePipe();
    void handleQueue();
    bool hasWork();
    void closeConn();

    int id;
    bool wantToToQuit;
    bool terminate;
    const int mainFd;
    const int pipeFd;
    JobQueue jobQueue;
    IOWorkerExitCb exitCb;
    IOWorkerTimeoutCb timeoutCb;
    IOWorkerExecuteSafeCb execCb;
    ErrArr errs;
    IOErrClass mainSockErr;
    bool closedFd;
    NetAddress ownAddr;
    NetAddress clientAddr;
    std::shared_ptr<TimeVal> responseTimeout;
    int timeout;
    int nextTimeout;
    std::queue<std::string> pendingIncoming;
    std::queue<SSendJob> pendingOutgoing;
    Logger& logger;
    bool peerCorrupted;
    pollfd* pollFds;
};

using SIOWorker = std::shared_ptr<IOWorker>;

#endif //KIERKI_IO_WORKER_H
