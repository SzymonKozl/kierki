//
// Created by szymon on 12.05.24.
//

#include <iostream>
#include "io_worker_mgr.h"
#include "constants.h"
#include "common_types.h"

#include "unistd.h"
#include "cerrno"
#include "thread"
#include "memory"


IOWorkerMgr::IOWorkerMgr(IOWorkerMgrPipeCb &&pipeCb):
        workers(),
        pipes(),
        threads(),
        nextIx(0),
        pipeCb(pipeCb),
        clearThreadsSemaphore{0},
        threadsStructuresMutex(),
        toErase(),
        finishFlag(false)
{}

IOWorkerMgr::~IOWorkerMgr() {
    for (auto & thread : threads) {
        int ix = thread.first;
        sendKill(ix);
        close(pipes[ix].first);
        close(pipes[ix].second);
    }
}

void IOWorkerMgr::signal(int ix, bool locked) {
    if (locked) {
        char buff = 42;
        ssize_t ret = write(pipes[ix].first, &buff, 1);
        if (ret < 0) {
            pipeCb({"write", errno, IO_ERR_INTERNAL});
        }
    }
    else {
        MutexGuard lock(threadsStructuresMutex);
        char buff = 42;
        ssize_t ret = write(pipes[ix].first, &buff, 1);
        if (ret < 0) {
            pipeCb({"write", errno, IO_ERR_INTERNAL});
        }
    }
}

void IOWorkerMgr::sendKill(int ix, bool locked) {
    if (!locked) {
        MutexGuard lock(threadsStructuresMutex);
        workers.at(ix)->scheduleDeath();
        signal(ix);
    }
    else {
        workers.at(ix)->scheduleDeath();
        signal(ix);
    }
}

void IOWorkerMgr::finish() {
    MutexGuard lock(threadsStructuresMutex);
    for (auto & worker : workers) {
        if (std::find(toErase.begin(), toErase.end(), worker.first) != toErase.end()) {
            continue;
        }
        sendKill(worker.first, true);
        toErase.push_back(worker.first);
    }
    clearThreadsSemaphore.release();
}

void IOWorkerMgr::sendJob(SSendJob job, int ix) {
    MutexGuard lock(threadsStructuresMutex);
    workers.at(ix)->newJob(job);
    signal(ix);
}

void IOWorkerMgr::eraseWorker(int ix) {
    MutexGuard lock(threadsStructuresMutex);
    if (std::find(toErase.begin(), toErase.end(), ix) != toErase.end()) {
        return;
    }
    toErase.push_back(ix);
    clearThreadsSemaphore.release();
}

void IOWorkerMgr::waitForClearing() {
    bool exitFlag = false;
    std::vector<Sjthread> q;
    std::vector<SIOWorker> workers_to_erase;
    while (!exitFlag) {
        clearThreadsSemaphore.acquire();
        { // mutex lock lifetime scope
            MutexGuard lock(threadsStructuresMutex);
            while (!toErase.empty()) {
                int ix = toErase.back();
                q.push_back(threads[ix]);
                workers_to_erase.push_back(workers[ix]);
                threads.erase(ix);
                workers.erase(ix);
                // todo pipe cleanup
                toErase.pop_back();
            }
            if (finishFlag && workers.empty()) exitFlag = true;
        }
        q.clear(); // destroying jthreads outside mutex scope to prevent holding mutex during join
        workers_to_erase.clear();
    }
}

void IOWorkerMgr::releaseCleaner() {
    MutexGuard lock(threadsStructuresMutex);
    finishFlag = true;
}

void IOWorkerMgr::clearPipes(int ix) {
    MutexGuard lock(threadsStructuresMutex);
    if (close(pipes[ix].first)) pipeCb({"close", 0, IO_ERR_EXTERNAL});
    if (close(pipes[ix].second)) pipeCb({"close", 0, IO_ERR_EXTERNAL});
}

void IOWorkerMgr::setRole(int ix, WorkerRole role) {
    MutexGuard lock(threadsStructuresMutex);
    if (roles.find(ix) == roles.end()) {
        throw std::runtime_error("tried to set role for non-existing worker\n");
    }
    roles[ix] = role;
}

WorkerRole IOWorkerMgr::getRole(int ix) {
    MutexGuard lock(threadsStructuresMutex);
    return roles.at(ix);
}
