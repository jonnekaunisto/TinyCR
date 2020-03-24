#include "perf_tool.h"

LatencyStatistics::LatencyStatistics(){}

void LatencyStatistics::addStatistic(std::string statistic)
{
    statisticsMap[statistic] = std::pair<double, int>(0, 0);
}

void LatencyStatistics::addLatency(std::string statistic, double latency)
{
    statisticsMap[statistic].first += latency;
    statisticsMap[statistic].second++;
}
double LatencyStatistics::getAverageLatency(std::string statistic)
{
    if (statisticsMap.find(statistic) == statisticsMap.end())
    {
        return -2;
    }
    if (statisticsMap[statistic].second != 0)
    {
        return statisticsMap[statistic].first / statisticsMap[statistic].second;
    }
    else
    {
        return -1;
    }
}


StopWatch::StopWatch()
{
    startTime = std::chrono::high_resolution_clock::now();
}

double StopWatch::stop()
{
    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = finish - startTime;
    return duration.count();
}