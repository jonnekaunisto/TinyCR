#include "perf_tool.h"

LatencyStatistics::LatencyStatistics(){}

void LatencyStatistics::addStatistic(std::string statistic)
{
    statisticsMap[statistic] = std::pair<double, int>(0, 0);
}

bool LatencyStatistics::addLatency(std::string statistic, double latency)
{
    if (statisticsMap.find(statistic) != statisticsMap.end())
        {
            statisticsMap[statistic].first += latency;
            statisticsMap[statistic].second++;
            return true;
        }
    else
    {
        return false;
    }
}
double LatencyStatistics::getStatistic(std::string statistic)
{
    if (statisticsMap.find(statistic) != statisticsMap.end())
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
    return (finish - startTime).count();
}