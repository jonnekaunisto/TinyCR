#include "perf_tool.h"


StopWatch::StopWatch()
{
    startTime = std::chrono::high_resolution_clock::now();
}

double StopWatch::stop()
{
    auto finish = std::chrono::high_resolution_clock::now();
    return (finish - startTime).count();
}