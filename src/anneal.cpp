#include "scheduler.h"

int FinalScheduler::gendisturb(double temp)
{
    int n = need_schedule.size();
    int modifiy = std::max(temp * n * rand() / RAND_MAX, (double)n / 2);
    int start = n - modifiy;
    if (start == n)
        return -1;
    return start + rand() % modifiy;
}

double FinalScheduler::change(int pos, double credits, int *driver_volume, int *driver_capacity, bool &changed, std::vector<int>& middleMatchDriver)
{
    valRequest &rq = need_schedule[pos];
    if (matchDriver[pos] == -1)
    {
        changed = false;
        for (int i = 0; i < rq.request.len_Driver; i++)
        {
            int drID = rq.request.Driver[i];
            if (driver_volume[drID] + rq.request.RequestSize <= driver_capacity[drID])
            {
                credits += rq.val * rq.request.RequestSize;
                driver_volume[drID] += rq.request.RequestSize;
                matchDriver[pos] = drID;
                changed = true;
                break;
            }
        }
    }
    else
    {
        assert(matchDriver[pos] == middleMatchDriver[pos]);
        changed = true;
        credits -= rq.val * rq.request.RequestSize;
        driver_volume[matchDriver[pos]] -= rq.request.RequestSize;
        matchDriver[pos] = -1;
    }
    return credits;
}

void FinalScheduler::recover(int pos, int *driver_volume, int *driver_capacity, std::vector<int>& middleMatchDriver)
{
    valRequest &rq = need_schedule[pos];
    if (matchDriver[pos] == -1)
    {
        matchDriver[pos] = middleMatchDriver[pos];
        driver_volume[matchDriver[pos]] += rq.request.RequestSize;
        // assert(driver_volume[matchDriver[pos]] <= driver_capacity[matchDriver[pos]]);
    }
    else
    {
        driver_volume[matchDriver[pos]] -= rq.request.RequestSize;
        matchDriver[pos] = -1;
    }
}

double FinalScheduler::disturb(int *driver_volume, int *driver_capacity, double temp, const std::vector<int> &p)
{
    int n = need_schedule.size();
    int modifiy = temp * n * rand() / RAND_MAX;
    int start = n - modifiy;

    double credits = 0;

    for (int i = 0; i < n; i++)
        matchDriver[i] = -1;

    for (int idx = 0; idx < start; idx++)
    {
        matchDriver[idx] = finalMatchDriver[idx];
        valRequest &rq = need_schedule[idx];
        if (matchDriver[idx] != -1)
        {
            driver_volume[matchDriver[idx]] += rq.request.RequestSize;
            credits += rq.val * rq.request.RequestSize;
        }
    }

    for (int i = 0; i < n; i++)
    {
        int idx = p[i];
        if (matchDriver[idx] != -1)
            continue;
        valRequest &rq = need_schedule[idx];

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

void FinalScheduler::solveSA(double &bestAns, int *driver_volume, int *driver_capacity) {
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

    double middleAns = bestAns;
    std::vector<int> middleMatchDriver = finalMatchDriver;
    auto startMatchDriver = middleMatchDriver;

    const int sa_time = 4;

    for (int times = sa_time, n = need_schedule.size(); times--;)
    {
        if (times > sa_time / 2)
            middleMatchDriver = startMatchDriver;
        else
            middleMatchDriver = finalMatchDriver;

        middleAns = 0;

        for (int i = 0; i < _driver_num; i++)
            driver_volume[i] = 0;

        for (int j = 0; j < need_schedule.size(); j++)
        {
            matchDriver[j] = middleMatchDriver[j];
            if (matchDriver[j] != -1)
            {
                valRequest &rq = need_schedule[j];
                driver_volume[matchDriver[j]] += rq.request.RequestSize;
                middleAns += rq.val * rq.request.RequestSize;
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
            k = std::min(k, 10); // 2277965.0
            k = std::max(k, 1);
            double nowans = middleAns;
            while (k--)
            {
                int pos = rand() % n;

                if (changelist.count(pos))
                    continue;

                nowans = change(pos, nowans, driver_volume, driver_capacity, changed, middleMatchDriver);
                if (changed)
                    changelist.insert(pos);
            }
            double delta = middleAns - nowans;
            if (delta < 0 || exp(-delta / temp) * RAND_MAX > rand())
            {
                middleAns = nowans;
                for (int pos : changelist)
                    middleMatchDriver[pos] = matchDriver[pos];
            }
            else
            {
                for (int pos : changelist)
                    recover(pos, driver_volume, driver_capacity, middleMatchDriver);
            }
            temp *= velo;
        }
        if (middleAns > bestAns)
        {
            bestAns = middleAns;
            finalMatchDriver = middleMatchDriver;
            vector<int>().swap(middleMatchDriver);
        }
    }

}
