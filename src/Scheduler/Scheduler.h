#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>

using SchedulerTaskHandler = void (*)();

struct SchedulerTask {
    SchedulerTaskHandler handler;
    unsigned long interval;
    unsigned long lastRun;
    bool active;
};

class Scheduler {
public:
    Scheduler();

    bool addTask(SchedulerTaskHandler handler, unsigned long interval, bool runImmediately = false);
    void run();
    void reset();

private:
    static const uint8_t MAX_TASKS = 8;
    SchedulerTask _tasks[MAX_TASKS];
    uint8_t _taskCount;
};

#endif // SCHEDULER_H
