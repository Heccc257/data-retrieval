#include <iostream>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <cstring>
#include <map>
#include <set>
#include <vector>
#include <random>
#include <cassert>
#include <ctime>
#include "scheduler.h"

using namespace std;


Result* FinalScheduler::C_schedule(int logical_clock, Request *request_list, int len_request, Driver *driver_list, int len_driver)
{
/********************** Pre-processing **********************/ 
    cerr << "time: " << logical_clock << " schedule begin\n";

    int *driver_volume = new int[_driver_num];
    int *driver_capacity = new int[_driver_num];

    get_need_schedule(logical_clock, request_list, len_request, need_schedule);

    memset(driver_volume, 0, sizeof(int) * _driver_num);
    memset(driver_capacity, 0, sizeof(int) * _driver_num);

    // result[i]表示标号为i的driver的结果
    for (int i = 0; i < _driver_num; i++)
    {
        result[i].DriverID = i;
        result[i].LogicalClock = logical_clock;
        result[i].len_RequestList = 0;
        // driver_capacity[i] = driver_list[i].Capacity;
    }
    for (int i = 0; i < len_driver; i++)
    {
        Driver &dr = driver_list[i];
        driver_capacity[dr.DriverID] = dr.Capacity;
    }

    vector<int>().swap(matchDriver);
    vector<int>().swap(finalMatchDriver);
    vector<int>().swap(survive);
    matchDriver.resize(need_schedule.size());
    finalMatchDriver.resize(need_schedule.size());
    survive.resize(need_schedule.size());

    double bestAns = -1e9;

/********************** Processing **********************/
    solveGreedy(bestAns, driver_volume, driver_capacity);

    double credits = 0;

    for (int i = 0; i < _driver_num; i++)
        driver_volume[i] = 0;

    for (int j = 0; j < need_schedule.size(); j++)
    {
        matchDriver[j] = finalMatchDriver[j];
        if (matchDriver[j] != -1)
        {
            valRequest &rq = need_schedule[j];
            driver_volume[matchDriver[j]] += rq.request.RequestSize;
        }
    }

    double middleans = bestAns;
    std::vector<int> middleMatchDriver = finalMatchDriver;

    double startans = middleans;
    auto startMatchDriver = middleMatchDriver;

    const int sa_time = 4;

    for (int times = sa_time, n = need_schedule.size(); times--;)
    {
        if (times > sa_time / 2)
        {
            bestAns = startans;
            finalMatchDriver = startMatchDriver;
        }
        else
        {
            bestAns = middleans;
            finalMatchDriver = middleMatchDriver;
        }
        matchDriver = finalMatchDriver;

        for (int i = 0; i < _driver_num; i++)
            driver_volume[i] = 0;

        for (int j = 0; j < need_schedule.size(); j++)
        {
            matchDriver[j] = finalMatchDriver[j];
            if (matchDriver[j] != -1)
            {
                valRequest &rq = need_schedule[j];
                driver_volume[matchDriver[j]] += rq.request.RequestSize;
            }
        }

        std::random_device rd;
        srand(rd());
        double temp = 1;
        double velo = 0.997;
        double momentum = 0.99;
        bool changed;
        while (temp > 1e-14)
        {
            std::set<int> changelist;
            int k = n * temp;
            // k = std::min(k, 1); //-2283673253.5
            k = std::min(k, 100); // 2277965.0
            k = std::max(k, 1);
            double nowans = bestAns;
            while (k--)
            {
                int pos = rand() % n;

                if (changelist.count(pos))
                    continue;

                nowans = change(pos, nowans, driver_volume, driver_capacity, changed);
                if (changed)
                    changelist.insert(pos);
            }
            double delta = bestAns - nowans;
            if (delta < 0 || exp(-delta / temp) * RAND_MAX > rand())
            {
                bestAns = nowans;
                for (int pos : changelist)
                    finalMatchDriver[pos] = matchDriver[pos];
            }
            else
            {
                for (int pos : changelist)
                    recover(pos, driver_volume, driver_capacity);
            }
            temp *= velo;
        }
        if (bestAns > middleans)
        {
            vector<int>().swap(middleMatchDriver);
            middleans = bestAns;
            middleMatchDriver = finalMatchDriver;
        }
    }

    finalMatchDriver = middleMatchDriver;

/********************** Post-processing **********************/ 
    for (int i = 0; i < _driver_num; i++)
        driver_volume[i] = 0;

    for (int i = 0; i < need_schedule.size(); i++)
        if (finalMatchDriver[i] != -1)
            driver_volume[finalMatchDriver[i]] += need_schedule[i].request.RequestSize;

    for (int idx = 0; idx < need_schedule.size(); idx++)
        if (finalMatchDriver[idx] != -1)
        {
            valRequest &rq = need_schedule[idx];
            for (int i = 0; i < rq.request.len_Driver; i++)
            {
                int drID = rq.request.Driver[i];
                if (driver_volume[drID] + rq.request.RequestSize <= driver_capacity[drID])
                {
                    driver_volume[drID] += rq.request.RequestSize;
                    finalMatchDriver[idx] = drID;
                    break;
                }
            }
        }

    for (int i = 0; i < _driver_num; i++)
    {
        assert(driver_volume[i] <= driver_capacity[i]);
        driver_volume[i] = 0;
    }

    std::vector<int>().swap(middleMatchDriver);

    // 分配内存

    for (int i = 0; i < _driver_num; i++)
        procNum[i] = 0;
    for (auto dr : finalMatchDriver)
        if (dr != -1)
            procNum[dr]++;

    for (int i = 0; i < _driver_num; i++)
    {
        if (result[i].RequestList != nullptr)
            delete[] result[i].RequestList;
        if (procNum[i])
            result[i].RequestList = new int[procNum[i]];
        else
            result[i].RequestList = nullptr;
    }
    matchDriver2Result(result, finalMatchDriver, need_schedule, driver_volume, driver_capacity);

    // 删除已经处理的请求
    for (int idx = 0; idx < need_schedule.size(); idx++)
    {
        if (finalMatchDriver[idx] != -1)
        {
            // del
            need_schedule[idx].request.SLA = -100;
        }
    }

    delete[] driver_volume;
    delete[] driver_capacity;

    return result;
}


extern "C"
{
    FinalScheduler *C_new() { return new FinalScheduler(); }
    void C_gc(FinalScheduler *self)
    {
        if (self == nullptr)
        {
            cerr << "CTMD\n";
        }
        else
            delete self;
    }
    void C_init(FinalScheduler *self, int driver_num) { return self->C_init(driver_num); }
    Result *C_schedule(FinalScheduler *self, int logical_clock, Request *request_list, int len_request, Driver *driver_list, int len_driver)
    {
        return self->C_schedule(logical_clock, request_list, len_request, driver_list, len_driver);
    }
}
