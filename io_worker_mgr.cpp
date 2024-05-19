//
// Created by szymon on 12.05.24.
//

#include "io_worker_mgr.h"
#include "constants.h"

#include "unistd.h"
#include "cerrno"
#include "thread"
#include "memory"


IOWorkerMgr::IOWorkerMgr(IOWorkerMgrPipeCb &&pipeCb):
    workers(),
    pipes(),
    threads(),
    nextIx(0),
    pipeCb(pipeCb)
{}

IOWorkerMgr::~IOWorkerMgr() {
    for (auto & thread : threads) {
        int ix = thread.first;
        sendKill(ix);
        close(pipes[ix].first);
        close(pipes[ix].second);
    }
}

void IOWorkerMgr::signal(int ix) {
    char buff = 42;
    ssize_t ret = write(pipes[ix].first, &buff, 1);
    if (ret < 0) {
        pipeCb({"write", errno, IO_ERR_INTERNAL});
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
        pipeCb({"close", errno, IO_ERR_INTERNAL});
    }
}

void IOWorkerMgr::joinThread(int ix) {
    threads[ix].join();
}

void IOWorkerMgr::halt() {
    for (auto& worker: workers) {
        worker.second->halt();
    }
}

void IOWorkerMgr::unhalt() {
    for (auto& worker: workers) {
        worker.second->unhalt();
    }
}
