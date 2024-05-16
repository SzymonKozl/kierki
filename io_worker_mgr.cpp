//
// Created by szymon on 12.05.24.
//

#include "io_worker_mgr.h"

#include "unistd.h"
#include "cerrno"
#include "thread"


IOWorkerMgr::IOWorkerMgr(IOWorkerMgrPipeCb &&pipeCb):
    workers(),
    nextIx(0),
    pipeCb(pipeCb),
    pipes(),
    threads()
{}

IOWorkerMgr::~IOWorkerMgr() {
    for (auto it = threads.begin(); it != threads.end(); it ++) {
        int ix = it->first;
        sendKill(ix);
        close(pipes[ix].first);
        close(pipes[ix].second);
    }
}

template<class T, class ...Args>
requires std::is_base_of_v<IOWorker, T>
void IOWorkerMgr::spawnNewWorker(Args ...args) {
    int ix = nextIx ++;
    int pipe_fd[2];
    int ret = pipe(pipe_fd);
    if (ret < 0) {
        pipeCb("pipe", errno);
    }

    pipes.insert(std::make_pair(ix, std::make_pair(pipe_fd[1], pipe_fd[0])));
    workers.insert(std::make_pair(ix, T(pipe_fd[0], ix, args...)));

    threads.insert(std::make_pair(ix, std::jthread([&]() {workers.at(ix).run();})));
}

void IOWorkerMgr::signal(int ix) {
    char buff = 42;
    int ret = write(pipes[ix].first, &buff, 1);
    if (ret < 0) {
        pipeCb("write", errno);
    }
}

void IOWorkerMgr::sendKill(int ix) {
    workers.at(ix).scheduleDeath();
    signal(ix);
}

void IOWorkerMgr::killAll() {
    for (auto it = workers.begin(); it != workers.end(); it ++) {
        sendKill(it->first);
    }
}

void IOWorkerMgr::sendJob(SendJob &job, int ix) {
    workers.at(ix).newJob(job);
    signal(ix);
}

void IOWorkerMgr::eraseWorker(int ix) {
    workers.erase(ix);
    threads.erase(ix);
    if (close(pipes.at(ix).first) || close(pipes.at(ix).second)) {
        pipeCb("close", errno);
    }
}