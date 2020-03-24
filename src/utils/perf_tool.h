/**
 * Holds Stopwatch Class
 * @author Jonne Kaunisto
 */
#ifndef PERF_TOOL_H
#define PERF_TOOL_H
#include <chrono>


class StopWatch {

public:
    StopWatch();

    double stop();

private:
    std::chrono::high_resolution_clock::time_point startTime;
    double duration;
};

#endif