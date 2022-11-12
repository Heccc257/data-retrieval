#include <iostream>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <cstring>
#include <map>
#include <set>
#include <vector>

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
    Request() { }
};

enum rqTYPE {
    FE = 0,
    BE = 1,
    EM = 2
};
/**
 * valRequest 对Request进行估值操作
 * 
 */
struct valRequest{
    Request request;
    int nowLogicalClock;
    double val;
    bool scheduled;
    valRequest() { }
    valRequest(int _now, const Request &rq) {
        request = rq;
        request.Driver = new int[request.len_Driver];
        memcpy(request.Driver, rq.Driver, sizeof(int) * request.len_Driver);
        nowLogicalClock = _now;
        val = calc_val();
    }
    bool operator < (const valRequest& rq)const {
        if(fabs(val - rq.val) > 0.001) return val > rq.val;
        return request.RequestSize > rq.request.RequestSize;
    }

    double calc_val() {
        // 若在当前时刻处理该请求,超时的时间数
        // 负数表示还未超时
        int timeout = nowLogicalClock - (request.LogicalClock + request.SLA - 1);
        if(request.RequestType == BE) {
            // 后台取回必须<12
            if(timeout >= 0) return -1;
            else return 0.5 * ceil(1.0*request.RequestSize / 50) / request.RequestSize;
        }

        // 超时达到了12,直接丢弃
        if(timeout >= 12) return -1;

        double coe = (request.RequestType == FE) ? 1 : 2;
        if(timeout >= 0) {
            return coe * ceil(1.0*request.RequestSize / 50);
        } else {
            return (0.3 / abs(timeout)) * coe * ceil(1.0*request.RequestSize / 50);
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
};

struct NestResult
{
    int len_Result;
    Result *result;
};

class CScheduler
{

public:
    CScheduler() {
        need_schedule.clear();
    }
    ~CScheduler() {}
    void C_init(int driver_num)
    {
        _driver_num = driver_num;
    }

    void get_need_schedule(int logical_clock, Request *request_list, int len_request) {
        vector<Request>rq;
        rq.clear();
        for(auto c : need_schedule) {
            rq.push_back(c.request);
        }
        for(int i=0; i<len_request; i++)
            rq.push_back(*(request_list+i));

        need_schedule.clear();
        for(auto r : rq) {
            need_schedule.push_back(valRequest(logical_clock, r));
        }
        sort(need_schedule.begin(), need_schedule.end());
        
        while(need_schedule.size() && need_schedule.back().val < 0) {
            need_schedule.pop_back();
        }
    }

    void matchDriver2Result(Result* result, vector<int>& matchDriver, vector<valRequest>& need_schedule) {
        for (int i = 0; i < _driver_num; i++)
            result[i].len_RequestList = 0;
        for(int i=0; i<need_schedule.size(); i++) {
            int drID = matchDriver[i];
            if(drID != -1) 
                result[drID].RequestList[ result[drID].len_RequestList++ ] = need_schedule[i].request.RequestID;
        }
    }

    double solve(int *driver_volume, int *driver_capacity) {
        // cout <<"solve begin size = " << need_schedule.size() << '\n';
        for(int i=0; i < _driver_num; i++) 
            driver_volume[i] = 0;
        srand(time(NULL));
        double credits = 0;
        double deny = 1;
        // cout << "preproc\n";  
        for(int idx=0; idx < need_schedule.size(); idx++) {
            valRequest &rq = need_schedule[idx];
            matchDriver[idx] = -1;
            if(rq.request.len_Driver == 0) continue ;

            int bgDriver = rand()%rq.request.len_Driver;
            for(int i=0; i<rq.request.len_Driver; i++) {
                int drID = rq.request.Driver[ (i + bgDriver) % rq.request.len_Driver ];
                if(driver_volume[drID] + rq.request.RequestSize <= driver_capacity[drID]) {
                    credits += rq.val * rq.request.RequestSize;
                    driver_volume[drID] += rq.request.RequestSize;
                    matchDriver[idx] = drID;
                    break;
                }
            }
            deny *= 0.99;
        }
        return credits;
    }

    Result *C_schedule(int logical_clock, Request *request_list, int len_request, Driver *driver_list, int len_driver)
    {
        cout << "time: " << logical_clock << " schedule begin\n";
        Result *result = new Result[_driver_num];
        int *driver_volume = new int[_driver_num];
        int *driver_capacity = new int[_driver_num];

        memset(driver_volume, 0, sizeof(int) * _driver_num);

        for (int i = 0; i < _driver_num; i++)
        {
            result[i].DriverID = driver_list[i].DriverID;
            result[i].LogicalClock = logical_clock;
            result[i].len_RequestList = 0;
            result[i].RequestList = new int[len_request];
            driver_capacity[i] = driver_list[i].Capacity;
        }

        get_need_schedule(logical_clock, request_list, len_request);
        cout << need_schedule.size() << " get schedule end\n";
        // for(auto rq : need_schedule) {
        //     cout << rq.request.RequestID << '\n';
        // }
        matchDriver.resize(need_schedule.size());
        finalMatchDriver.resize(need_schedule.size());

        double bestAns = -1e9;
        for(int i=0; i<500; i++) {
            double nowans = solve(driver_volume, driver_capacity);
            if(nowans > bestAns) {
                bestAns = nowans;
                matchDriver2Result(result, matchDriver, need_schedule);
                finalMatchDriver = matchDriver;
            }
        }
        cout << "solve end\n";
        
        // 删除已经处理的请求
        for(int idx=0; idx < need_schedule.size(); idx++) {
            if(finalMatchDriver[idx] != -1) {
                need_schedule[idx].request.SLA = -100;
            }
        }
        for(int i=0; i<_driver_num; i++) {
            cout<<"res : " << result[i].len_RequestList << '\n';
        }
        cout << "delete end\n";
        return result;
    }

private:
    int _driver_num;
    vector<valRequest>need_schedule;
    vector<int>matchDriver; // -1表示不处理
    vector<int>finalMatchDriver; 

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
    CScheduler *C_new() { return new CScheduler(); }
    void C_init(CScheduler *self, int driver_num) { return self->C_init(driver_num); }
    Result *C_schedule(CScheduler *self, int logical_clock, Request *request_list, int len_request, Driver *driver_list, int len_driver)
    {
        return self->C_schedule(logical_clock, request_list, len_request, driver_list, len_driver);
    }
}