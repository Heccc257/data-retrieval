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

using namespace std;
struct Request
{
    int RequestID;
    // FE:0, BE:1, EM:2
    int RequestType;
    int RequestSize;
    int LogicalClock;
    int SLA;
    int len_Driver;
    int *Driver;

    Request() { Driver = nullptr; }

    // 不要析构
    ~Request()
    {
        if (Driver != nullptr)
            delete[] Driver;
    }
    Request(const Request &rhs)
    {
        if (this == &rhs)
            return;
        RequestID = rhs.RequestID;
        RequestSize = rhs.RequestSize;
        RequestType = rhs.RequestType;
        LogicalClock = rhs.LogicalClock;
        SLA = rhs.SLA;
        len_Driver = rhs.len_Driver;
        Driver = new int[len_Driver];
        memcpy(Driver, rhs.Driver, sizeof(int) * len_Driver);
    }
    Request &operator=(const Request &rhs)
    {
        if (this == &rhs)
            return *this;
        RequestID = rhs.RequestID;
        RequestSize = rhs.RequestSize;
        RequestType = rhs.RequestType;
        LogicalClock = rhs.LogicalClock;
        SLA = rhs.SLA;
        len_Driver = rhs.len_Driver;
        Driver = new int[len_Driver];
        memcpy(Driver, rhs.Driver, sizeof(int) * len_Driver);
        return *this;
    }
};

enum rqTYPE
{
    FE = 0,
    BE = 1,
    EM = 2
};
/**
 * valRequest 对Request进行估值操作
 *
 */
struct valRequest
{
    Request request;
    int nowLogicalClock;
    int timeout;
    double val;
    valRequest() {}
    valRequest(int _now, const Request &rq)
    {
        // request = rq;
        // request.Driver = new int[request.len_Driver];
        // memcpy(request.Driver, rq.Driver, sizeof(int) * request.len_Driver);
        request = rq;
        nowLogicalClock = _now;
        // if(request.SLA < 0) {
        //     cout <<"id = " << request.RequestID <<" val = " << calc_val() << '\n';
        // }
        val = calc_val();
    }
    ~valRequest()
    {
        // if(request.Driver != nullptr)
        // delete request.Driver;
    }
    bool operator<(const valRequest &rq) const
    {
        if (fabs(val - rq.val) > 0.001)
            return val > rq.val;
        else
            return request.len_Driver < rq.request.len_Driver;
    }

    double calc_val()
    {
        // size=0,这合理吗???
        if (request.RequestSize == 0)
            return -1;
        // 若在当前时刻处理该请求,超时的时间数
        // 负数表示还未超时
        timeout = nowLogicalClock - (request.LogicalClock + request.SLA - 1);

        // 超时达到了12,直接丢弃
        if (timeout >= 12)
            return -1;

        // EM立即处理
        if (request.RequestType == EM)
            return 2.0;

        // 后台取回必须<12
        if (request.RequestType == BE)
        {
            if (timeout > 0)
                return -1;
            else if (timeout > -6)
                return 0.5 * ceil(1.0 * request.RequestSize / 50) / request.RequestSize;
            else
                return 0.2 * ceil(1.0 * request.RequestSize / 50) / request.RequestSize; // 还不太紧急
        }

        // 前台
        double coe = 1;

        if (timeout >= 0)
        {
            return coe * ceil(1.0 * request.RequestSize / 50) / request.RequestSize;
        }
        else
        {
            return (0.4 + 0.4 * (12 - abs(timeout)) / 12) * coe * ceil(1.0 * request.RequestSize / 50) / request.RequestSize;
        }
    }
};

struct Driver
{
    int DriverID;
    int Capacity;
    int LogicalClock;
};
struct Result
{
    int DriverID;
    int LogicalClock;
    int len_RequestList;
    int *RequestList;
    Result()
    {
        RequestList = nullptr;
    }
    ~Result() {}
};

struct NestResult
{
    int len_Result;
    Result *result;
};

class FinalScheduler
{

public:
    set<int> rqID;
    FinalScheduler() { need_schedule.clear(); }
    ~FinalScheduler() {}

    void C_init(int driver_num)
    {
        _driver_num = driver_num;
        result = new Result[_driver_num];
        procNum.resize(_driver_num);
    }

    void get_need_schedule(int logical_clock, Request *request_list, int len_request, vector<valRequest> &need_schedule)
    {
        // need_schedule.clear();

        // for(int i=0; i<len_request; i++)
        //     need_schedule.push_back(valRequest(logical_clock, *(request_list+i)));

        vector<Request> rq;
        for (auto c : need_schedule)
        {
            // cerr << "need_schedule : log = " << c.request.LogicalClock << '\n';
            rq.push_back(c.request);
        }
        for (int i = 0; i < len_request; i++)
            rq.push_back(*(request_list + i));

        need_schedule.clear();
        for (auto c : rq)
        {
            // cerr << "rq: log = " << c.LogicalClock << '\n';
            need_schedule.push_back(valRequest(logical_clock, c));
        }
        sort(need_schedule.begin(), need_schedule.end());
        while (need_schedule.size() && need_schedule.back().val < 0)
            need_schedule.pop_back();
        rq.clear();
    }

    void matchDriver2Result(Result *result, vector<int> &matchDriver, vector<valRequest> &need_schedule, int *a, int *b)
    {
        for (int i = 0; i < _driver_num; i++)
            result[i].len_RequestList = 0;
        for (int i = 0; i < need_schedule.size(); i++)
        {
            int drID = matchDriver[i];
            if (drID != -1)
            {
                result[drID].RequestList[result[drID].len_RequestList++] = need_schedule[i].request.RequestID;
                a[drID] += need_schedule[i].request.RequestSize;
                assert(a[drID] <= b[drID]);
            }
        }
        // for (int i = 0; i < _driver_num; i++)
        // assert(result[i].len_RequestList == procNum[i]);
    }

    // double solveGreedy(int *driver_volume, int *driver_capacity);

    Result *C_schedule(int logical_clock, Request *request_list, int len_request, Driver *driver_list, int len_driver)
    {
        cerr << "time: " << logical_clock << " schedule begin\n";
        // cerr << "len = " << len_driver << ' ' << _driver_num << '\n';
        // for (int i=0; i<len_driver; i++) {
        //     cerr << driver_list[i].DriverID << " " << driver_list[i].Capacity << '\n';
        // }

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

        // solve 贪心算法
        double bestAns = -1e9;
        int epochs = 50;
        for (int i = 0; i < epochs; i++)
        {
            double nowans = solveGreedy(driver_volume, driver_capacity);
            if (nowans > bestAns)
            {
                bestAns = nowans;

                // cerr << "match over\n";
                for (int j = 0; j < matchDriver.size(); j++)
                {
                    finalMatchDriver[j] = matchDriver[j];
                    if (finalMatchDriver[j] != -1)
                        survive[j]++;
                }
            }
        }

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

        const int sa_time = 128;

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
                int pos = rand() % n;
                // int pos = gendisturb(temp);
                if (pos == -1)
                    continue;
                double nowans = change(pos, bestAns, driver_volume, driver_capacity, changed);
                // double nowans = disturb(driver_volume, driver_capacity, temp, p);
                double delta = bestAns - nowans;
                if (delta < 0 || exp(-delta / temp) * RAND_MAX > rand())
                {
                    bestAns = nowans;
                    finalMatchDriver[pos] = matchDriver[pos];
                }
                else if (changed)
                {
                    recover(pos, driver_volume, driver_capacity);
                }
                // temp *= 0.997;
                temp *= velo;
                // if (velo > 0.8)
                // velo *= momentum;
                // cnt++;
            }
            if (bestAns > middleans)
            {
                vector<int>().swap(middleMatchDriver);
                middleans = bestAns;
                middleMatchDriver = finalMatchDriver;
            }
            // std::cerr << (clock() - start) / (double)CLOCKS_PER_SEC << '\n';
            // std::cerr << cnt << "\n";
        }

        finalMatchDriver = middleMatchDriver;

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
        // for (int i=0; i<_driver_num; i++) {
        //     cerr << result[i].len_RequestList << " ID = " << result[i].DriverID << ' ';
        //     for (int j=0; j < result[i].len_RequestList; j++)
        //         cerr << result[i].RequestList[j] << ' ';
        //     cerr << '\n';
        // }
        return result;
    }

    const long long mod = 1e9 + 7;
    int randGen(int &base, int p)
    {
        base = 1ll * base * p % mod;
        return base;
    }

    double solveGreedy(int *driver_volume, int *driver_capacity)
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
            if (1.0 * (randGen(base, p) % mod) / mod - survive[idx] / 20.0 > deny)
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
            if (deny > 0.8)
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

    int gendisturb(double temp)
    {
        int n = need_schedule.size();
        int modifiy = std::max(temp * n * rand() / RAND_MAX, (double)n / 2);
        int start = n - modifiy;
        if (start == n)
            return -1;
        return start + rand() % modifiy;
    }

    double change(int pos, double credits, int *driver_volume, int *driver_capacity, bool &changed)
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

    void recover(int pos, int *driver_volume, int *driver_capacity)
    {
        valRequest &rq = need_schedule[pos];
        if (matchDriver[pos] == -1)
        {
            matchDriver[pos] = finalMatchDriver[pos];
            driver_volume[matchDriver[pos]] += rq.request.RequestSize;
            assert(driver_volume[matchDriver[pos]] <= driver_capacity[matchDriver[pos]]);
        }
        else
        {
            driver_volume[matchDriver[pos]] -= rq.request.RequestSize;
            matchDriver[pos] = -1;
        }
    }

    double disturb(int *driver_volume, int *driver_capacity, double temp, const std::vector<int> &p)
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

private:
    int _driver_num;
    vector<valRequest> need_schedule;
    // 两个matchDriver的下标都是与need_schedule对应,注意不是requestID
    vector<int> matchDriver; // -1表示不处理
    vector<int> finalMatchDriver;

    // 每个dr需要处理的数量
    vector<int> procNum;
    // 表示某个询问在更优方案中被处理的次数,处理得越多越不容易被踢出
    // survive的下标都是与need_schedule对应,注意不是requestID
    vector<int> survive;
    Result *result;

    void My_algo(int logical_clock, Request *request_list, int len_request, Driver *driver_list, int len_driver, Result *result, int *driver_volume, int *driver_capacity)
    {
        for (int request = 0; request < len_request; request++)
        {
            for (int device = 0; device < request_list[request].len_Driver; device++)
            {
                if (driver_volume[request_list[request].Driver[device]] + request_list[request].RequestSize <= driver_capacity[request_list[request].Driver[device]])
                {
                    driver_volume[request_list[request].Driver[device]] += request_list[request].RequestSize;
                    result[request_list[request].Driver[device]].RequestList[result[request_list[request].Driver[device]].len_RequestList++] = request_list[request].RequestID;
                }
                break;
            }
        }
    }
};

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
