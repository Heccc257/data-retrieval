#ifndef SCHEDULER_H
#define SCHEDULER_H
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


#define LOGERR(output) cerr << output << '\n';
// #define LOGERR(output) if(false) while(false);
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
            // return 2.0 ;
            return 2.0 * ceil(1.0 * request.RequestSize / 50) / request.RequestSize;

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

    const long long mod = 1e9 + 7;
    int randGen(int &base, int p)
    {
        base = 1ll * base * p % mod;
        return base;
    }

    void ansCompete(double &bestAns, double &nowans, vector<int> &matchDriver, vector<int> &finalMatchDriver, vector<int> &survive) {
        if (nowans > bestAns)
        {
            bestAns = nowans;
            for (int j = 0; j < matchDriver.size(); j++)
            {
                finalMatchDriver[j] = matchDriver[j];
                if (finalMatchDriver[j] != -1)
                    survive[j]++;
            }
        }
    }

public:
    Result *C_schedule(int logical_clock, Request *request_list, int len_request, Driver *driver_list, int len_driver);

public:
    double solveGreedy(int *driver_volume, int *driver_capacity, int numProtect);
    void solveGreedy(double &bestAns, int *driver_volume, int *driver_capacity);

public:
    int gendisturb(double temp);
    double change(int pos, double credits, int *driver_volume, int *driver_capacity, bool &changed);
    void recover(int pos, int *driver_volume, int *driver_capacity);
    double disturb(int *driver_volume, int *driver_capacity, double temp, const std::vector<int> &p);



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
#endif // scheduler.h