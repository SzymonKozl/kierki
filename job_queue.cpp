//
// Created by szymon on 11.05.24.
//

#include "job_queue.h"
#include "common_types.h"

#include "mutex"
#include "stdexcept"
#include "cstddef"

JobQueue::JobQueue():
    killFlag(false),
    halt(false),
    jobsPending(),
    mutex()
{}

SSendJob JobQueue::popNextJob() {
    std::lock_guard<std::mutex> lock(mutex);
    if (jobsPending.empty()) throw std::runtime_error("tried to pop from empty job queue");
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

size_t JobQueue::jobCount() noexcept {
    MutexGuard lock(mutex);
    return jobsPending.size();
}

void JobQueue::setStopped(bool val) noexcept {
    MutexGuard lock(mutex);
    halt = val;
}

bool JobQueue::isStopped() noexcept {
    MutexGuard lock(mutex);
    return halt;
}
