//
// Created by szymon on 12.05.24.
//

#ifndef KIERKI_IO_WORKER_MGR_H
#define KIERKI_IO_WORKER_MGR_H

#include "io_worker.h"
#include "constants.h"

#include "unordered_map"
#include "concepts"
#include "functional"
#include "string"
#include "thread"
#include "memory"
#include "mutex"
#include "semaphore"

enum WorkerRole {
    HANDLING_ACTIVE,
    HANDLING_UNKNOWN,
    INCOMING_PROXY,
    HANDLING_DISCARDED
};

using IOWorkerMgrPipeCb = std::function<void(ErrInfo)>;

class IOWorkerMgr {
public:
    template<class T, class... Args> requires std::is_base_of_v<IOWorker, T> &&  std::constructible_from<T, int, int, Args...>
    int spawnNewWorker(WorkerRole role, Args... args);
    void sendKill(int ix, bool locked = false);
    void finish();
    void sendJob(SSendJob job, int ix);
    void eraseWorker(int ix);
    void waitForClearing();
    void releaseCleaner();
    void clearPipes(int ix);
    void setRole(int ix, WorkerRole role);
    WorkerRole getRole(int ix);
    explicit IOWorkerMgr(IOWorkerMgrPipeCb &&pipeCb);
    ~IOWorkerMgr();
private:
    using Sjthread = std::shared_ptr<std::jthread>;

    void signal(int ix);

    std::unordered_map<int, SIOWorker> workers;
    std::unordered_map<int, std::pair<int, int>> pipes;
    std::unordered_map<int, Sjthread> threads;
    std::unordered_map<int, WorkerRole> roles;
    int nextIx;
    IOWorkerMgrPipeCb pipeCb;

    std::binary_semaphore clearThreadsSemaphore;
    std::mutex threadsStructuresMutex;
    std::vector<int> toErase;
    bool finishFlag;
};

template<class T, class ...Args>
requires std::is_base_of_v<IOWorker, T> && std::constructible_from<T, int, int, Args...>
inline int IOWorkerMgr::spawnNewWorker(WorkerRole role, Args ...args) {
    MutexGuard lock(threadsStructuresMutex);
    int ix = nextIx++;
    int pipe_fd[2];
    int ret = pipe(pipe_fd);
    if (ret < 0) {
        pipeCb({"pipe", errno, IO_ERR_INTERNAL});
    }

    pipes.emplace(ix, std::make_pair(pipe_fd[1], pipe_fd[0]));
    workers.emplace(ix, std::static_pointer_cast<IOWorker>(std::make_shared<T>(pipe_fd[0], ix, args...)));

    threads.emplace(ix, std::make_shared<std::jthread>([ix, this]() { this->workers.at(ix)->run(); }));
    roles.emplace(ix, role);
    return ix;
}


#endif //KIERKI_IO_WORKER_MGR_H
