#include "scheduler.h"

double FinalScheduler::solveGreedy(int *driver_volume, int *driver_capacity, int numProtect)
{
    random_device rd;
    int base = rd() % mod;
    int p = rd() % mod;
    for (int i = 0; i < _driver_num; i++)
        driver_volume[i] = 0;
    srand(time(NULL));
    double credits = 0;
    double deny = 1;
    // cout << "preproc\n";

    // 初始化
    for (int idx = 0; idx < need_schedule.size(); idx++)
        matchDriver[idx] = -1;

    for (int idx = 0; idx < need_schedule.size(); idx++)
    {
        valRequest &rq = need_schedule[idx];
        if (rq.request.len_Driver == 0)
            continue;

        // 随机踢出
        if ( idx >= numProtect && (1.0 * (randGen(base, p) % mod) / mod - survive[idx] / 20.0 > deny) )
            continue;

        int bgDriver = randGen(base, p);

        for (int i = 0; i < rq.request.len_Driver; i++)
        {
            int drID = rq.request.Driver[(i + bgDriver) % rq.request.len_Driver];
            if (driver_volume[drID] + rq.request.RequestSize <= driver_capacity[drID])
            {

                credits += rq.val * rq.request.RequestSize;
                driver_volume[drID] += rq.request.RequestSize;
                matchDriver[idx] = drID;
                break;
            }
        }
        if (idx >= numProtect && deny > 0.75)
            deny *= 0.99;
    }

    for (int idx = 0; idx < need_schedule.size(); idx++)
    {
        if (matchDriver[idx] != -1)
            continue;
        valRequest &rq = need_schedule[idx];
        if (rq.request.len_Driver == 0)
            continue;

        for (int i = 0; i < rq.request.len_Driver; i++)
        {
            int drID = rq.request.Driver[i];
            if (driver_volume[drID] + rq.request.RequestSize <= driver_capacity[drID])
            {
                credits += rq.val * rq.request.RequestSize;
                driver_volume[drID] += rq.request.RequestSize;
                matchDriver[idx] = drID;
                break;
            }
        }
    }

    return credits;
}

void FinalScheduler::solveGreedy(double &bestAns, int *driver_volume, int *driver_capacity) {
    // solve 贪心算法
    int newBestTimes = 0;
    int epochs = 50;

    // 保护数量线性增加
    int numProtect = 0;
    for (int i = 0; i < epochs; i++)
    {
        double nowans = solveGreedy(driver_volume, driver_capacity, numProtect); 
        ansCompete(bestAns, nowans, matchDriver, finalMatchDriver, survive);
        numProtect += need_schedule.size() / epochs;
    }

    // 保护数量线性减少
    numProtect = need_schedule.size();
    for (int i = 0; i < epochs; i++)
    {
        double nowans = solveGreedy(driver_volume, driver_capacity, numProtect);
        ansCompete(bestAns, nowans, matchDriver, finalMatchDriver, survive);
        numProtect -= need_schedule.size() / epochs;
    }

    // 保护数量平方增加
    survive.clear();
    numProtect = 0;
    for (int i = 0; i < epochs; i++)
    {
        double nowans = solveGreedy(driver_volume, driver_capacity, numProtect);
        ansCompete(bestAns, nowans, matchDriver, finalMatchDriver, survive);
        numProtect = need_schedule.size() * pow(i+1, 2) / (epochs * epochs);
    }

    // 保护数量平方减少
    numProtect = need_schedule.size();
    for (int i = 0; i < epochs; i++)
    {
        double nowans = solveGreedy(driver_volume, driver_capacity, numProtect);
        ansCompete(bestAns, nowans, matchDriver, finalMatchDriver, survive);
        numProtect = need_schedule.size() * (pow(epochs-i-1, 2)) / (epochs * epochs);
    }

    // 保护数量平方随机
    std::random_device randomNumProtect;
    srand(randomNumProtect());
    survive.clear();
    for (int i = 0; i < epochs; i++)
    {
        numProtect = 1.0 * rand() / RAND_MAX * need_schedule.size();
        double nowans = solveGreedy(driver_volume, driver_capacity, numProtect);
        ansCompete(bestAns, nowans, matchDriver, finalMatchDriver, survive);
    }
    // LOGERR("new Best Times = " << newBestTimes)
}