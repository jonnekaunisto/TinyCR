/**
 * Holds Performance Tools Classes
 * @author Jonne Kaunisto
 */
#ifndef PERF_TOOL_H
#define PERF_TOOL_H
#include <chrono>
#include <string>
#include <unordered_map>
#include <utility>

class LatencyStatistics
{
public:
    LatencyStatistics();

    void addStatistic(std::string statistic);
    void addLatency(std::string statistic, double latency);
    double getAverageLatency(std::string statistic);

private:
    std::unordered_map<std::string, std::pair<double, int>> statisticsMap;
};


class StopWatch
{
public:
    StopWatch();

    double stop();

private:
    std::chrono::high_resolution_clock::time_point startTime;
    double duration;
};

#endif