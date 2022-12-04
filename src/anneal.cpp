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

double FinalScheduler::change(int pos, double credits, int *driver_volume, int *driver_capacity, bool &changed)
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
        assert(matchDriver[pos] == finalMatchDriver[pos]);
        changed = true;
        credits -= rq.val * rq.request.RequestSize;
        driver_volume[matchDriver[pos]] -= rq.request.RequestSize;
        matchDriver[pos] = -1;
    }
    return credits;
}

void FinalScheduler::recover(int pos, int *driver_volume, int *driver_capacity)
{
    valRequest &rq = need_schedule[pos];
    if (matchDriver[pos] == -1)
    {
        matchDriver[pos] = finalMatchDriver[pos];
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
