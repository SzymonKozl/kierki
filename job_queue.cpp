//
// Created by szymon on 11.05.24.
//

#include "job_queue.h"

#include "mutex"
#include "stdexcept"
#include "cstddef"

JobQueue::JobQueue():
    killFlag(false),
    jobsPending(),
    mutex()
{}

SendJob JobQueue::popNextJob() {
    std::lock_guard<std::mutex> lock(mutex);
    if (jobsPending.empty()) throw std::runtime_error("tried to pop from empty job queue");
    SendJob job = jobsPending.front();
    jobsPending.pop();
    return job;
}

bool JobQueue::hasKillOrder() noexcept {
    std::lock_guard<std::mutex> lock(mutex);
    return killFlag;
}

void JobQueue::pushNextJob(SendJob job) {
    std::lock_guard<std::mutex> lock(mutex);
    jobsPending.push(job);
}

const bool JobQueue::hasNextJob() noexcept {
    std::lock_guard<std::mutex> lock(mutex);
    return !jobsPending.empty();
}

void JobQueue::setKillOrder() noexcept {
    std::lock_guard<std::mutex> lock(mutex);
    killFlag = true;
}

size_t JobQueue::jobCount() {
    std::lock_guard<std::mutex> lock(mutex);
    return jobsPending.size();
}
