//
// Created by szymon on 12.05.24.
//

#ifndef KIERKI_IO_WORKER_MGR_H
#define KIERKI_IO_WORKER_MGR_H

#include "io_worker.h"
#include "unordered_map"

#include "concepts"
#include "functional"
#include "string"
#include "thread"
#include "memory"

using IOWorkerMgrPipeCb = std::function<void(std::string, int)>;

class IOWorkerMgr {
public:
    template<class T, class... Args> requires std::is_base_of_v<IOWorker, T> &&  std::constructible_from<T, int, int, Args...>
    int spawnNewWorker(Args... args);
    void sendKill(int ix);
    void killAll();
    void sendJob(SSendJob job, int ix);
    void eraseWorker(int ix);
    void joinThread(int ix);
    explicit IOWorkerMgr(IOWorkerMgrPipeCb &&pipeCb);
    ~IOWorkerMgr();
private:
    void signal(int ix);

    std::unordered_map<int, SIOWorker> workers;
    std::unordered_map<int, std::pair<int, int>> pipes;
    std::unordered_map<int, std::jthread> threads;
    int nextIx;
    IOWorkerMgrPipeCb pipeCb;
};

template<class T, class ...Args>
requires std::is_base_of_v<IOWorker, T> && std::constructible_from<T, int, int, Args...>
int IOWorkerMgr::spawnNewWorker(Args ...args) {
    int ix = nextIx++;
    int pipe_fd[2];
    int ret = pipe(pipe_fd);
    if (ret < 0) {
        pipeCb("pipe", errno);
    }

    pipes.insert(std::make_pair(ix, std::make_pair(pipe_fd[1], pipe_fd[0])));
    workers.insert(std::make_pair(ix, std::static_pointer_cast<IOWorker>(std::make_shared<T>(pipe_fd[0], ix, args...))));

    threads.insert(std::make_pair(ix, std::jthread([&]() { workers.at(ix)->run(); })));
    return ix;
}


#endif //KIERKI_IO_WORKER_MGR_H
