#include "Scheduler.h"

Scheduler::Scheduler() : _taskCount(0) {
    reset();
}

bool Scheduler::addTask(SchedulerTaskHandler handler, unsigned long interval, bool runImmediately) {
    if (_taskCount >= MAX_TASKS || handler == nullptr || interval == 0) {
        return false;
    }

    unsigned long now = millis();
    _tasks[_taskCount++] = {
        handler,
        interval,
        runImmediately ? now - interval : now,
        true
    };

    return true;
}

void Scheduler::run() {
    unsigned long now = millis();

    for (uint8_t i = 0; i < _taskCount; ++i) {
        SchedulerTask &task = _tasks[i];
        if (!task.active) {
            continue;
        }

        unsigned long elapsed = now - task.lastRun;
        if (elapsed >= task.interval) {
            task.lastRun = now;
            task.handler();
        }
    }
}

void Scheduler::reset() {
    _taskCount = 0;
    for (uint8_t i = 0; i < MAX_TASKS; ++i) {
        _tasks[i].handler = nullptr;
        _tasks[i].interval = 0;
        _tasks[i].lastRun = 0;
        _tasks[i].active = false;
    }
}
