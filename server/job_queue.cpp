//
// Created by szymon on 11.05.24.
//

#include "job_queue.h"

#include "mutex"
#include "stdexcept"

JobQueue::JobQueue():
    jobsPending(),
    mutex(),
    killFlag(false)
{}

SSendJob JobQueue::popNextJob() {
    std::lock_guard<std::mutex> lock(mutex);
    if (jobsPending.empty())
        throw std::runtime_error("tried to pop from empty job queue");
    SSendJob job = jobsPending.front();
    jobsPending.pop();
    return job;
}

bool JobQueue::hasKillOrder() noexcept {
    MutexGuard lock(mutex);
    return killFlag;
}

void JobQueue::pushNextJob(const SSendJob& job) {
    MutexGuard lock(mutex);
    jobsPending.push(job);
}

bool JobQueue::hasNextJob() noexcept {
    MutexGuard lock(mutex);
    return !jobsPending.empty();
}

void JobQueue::setKillOrder() noexcept {
    MutexGuard lock(mutex);
    killFlag = true;
}
