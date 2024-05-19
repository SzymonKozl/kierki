//
// Created by szymon on 12.05.24.
//

#include "io_worker_mgr.h"

#include <utility>

#include "unistd.h"
#include "cerrno"
#include "thread"
#include "memory"


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

void IOWorkerMgr::signal(int ix) {
    char buff = 42;
    ssize_t ret = write(pipes[ix].first, &buff, 1);
    if (ret < 0) {
        pipeCb("write", errno);
    }
}

void IOWorkerMgr::sendKill(int ix) {
    workers.at(ix)->scheduleDeath();
    signal(ix);
}

void IOWorkerMgr::killAll() {
    for (auto & worker : workers) {
        sendKill(worker.first);
    }
}

void IOWorkerMgr::sendJob(SSendJob job, int ix) {
    workers.at(ix)->newJob(std::move(job));
    signal(ix);
}

void IOWorkerMgr::eraseWorker(int ix) {
    workers.erase(ix);
    threads.erase(ix);
    if (close(pipes.at(ix).first) || close(pipes.at(ix).second)) {
        pipeCb("close", errno);
    }
}

void IOWorkerMgr::joinThread(int ix) {
    threads[ix].join();
}
