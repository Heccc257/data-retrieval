#include <iostream>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <cstring>
#include <map>
#include <set>
#include <vector>
#include <random>

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
    ~Request() {
        if(Driver != nullptr)
            delete []Driver;
    }
    Request(const Request& rhs) {
        if(this == &rhs) return ;
        RequestID = rhs.RequestID;
        RequestSize = rhs.RequestSize;
        RequestType = rhs.RequestType;
        LogicalClock = rhs.LogicalClock;
        SLA = rhs.SLA;
        len_Driver = rhs.len_Driver;
        Driver = new int[len_Driver];
        memcpy(Driver, rhs.Driver, sizeof(int) * len_Driver);
    }
    Request& operator=(const Request& rhs) {
        if(this == &rhs) return *this;
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

enum rqTYPE {
    FE = 0,
    BE = 1,
    EM = 2
};
/**
 * valRequest 对Request进行估值操作
 * 
 */
struct valRequest {
    Request request;
    int nowLogicalClock;
    int timeout;
    double val;
    valRequest() { }
    valRequest(int _now, const Request &rq) {
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
    ~valRequest() {
        // if(request.Driver != nullptr)
        //     delete request.Driver;
    }
    bool operator < (const valRequest& rq)const {
        if(fabs(val - rq.val) > 0.001) return val > rq.val;
        else return request.len_Driver < rq.request.len_Driver; 
    }

    double calc_val() {
        // size=0,这合理吗???
        if(request.RequestSize == 0) return -1;
        // 若在当前时刻处理该请求,超时的时间数
        // 负数表示还未超时
        timeout = nowLogicalClock - (request.LogicalClock + request.SLA - 1);
        if(request.RequestType == BE) {
            // 后台取回必须<12
            if(timeout > 0) return -1;
            else if(timeout >-6) return 0.5 * ceil(1.0*request.RequestSize / 50) / request.RequestSize;
            else return 0.2 * ceil(1.0*request.RequestSize / 50) / request.RequestSize; // 还不太紧急
        }

        // 超时达到了12,直接丢弃
        if(timeout >= 12) return -1;

        double coe = (request.RequestType == FE) ? 1 : 2;
        if(timeout >= 0) {
            return coe * ceil(1.0*request.RequestSize / 50);
        } else {
            return (0.3 * (12-abs(timeout)) / 12) * coe * ceil(1.0*request.RequestSize / 50);
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
    Result() {
        RequestList = nullptr;
    }
    ~Result() { }
};

struct NestResult
{
    int len_Result;
    Result *result;
};

class CScheduler
{

public:

    set<int>rqID;
    CScheduler() {need_schedule.clear(); }
    ~CScheduler() { }

    void C_init(int driver_num)
    {
        _driver_num = driver_num;
        result = new Result[_driver_num];
    }

    void get_need_schedule(int logical_clock, Request *request_list, int len_request) {
        // need_schedule.clear();

        // for(int i=0; i<len_request; i++)
        //     need_schedule.push_back(valRequest(logical_clock, *(request_list+i)));

        need_schedule.clear();
        vector<Request>rq;
        for(int i=0; i<len_request; i++)
            rq.push_back( *(request_list+i) );
        for(int i=0;i<len_request;i++)
            rq.push_back(request_list[i]);
        for(auto c : rq)
            need_schedule.push_back(valRequest(logical_clock, c));
        sort(need_schedule.begin(), need_schedule.end());
        // int totLen = need_schedule.size() + len_request;
        // Request* rq = new Request[totLen];

        // for(int i=0; i<len_request; i++)
        //     rq[i] = *(request_list+i);
        // for(int idx=0; idx<need_schedule.size(); idx++)
        //     rq[idx + len_request] = need_schedule[idx].request;

        // vector<valRequest>().swap(need_schedule);
        // need_schedule.resize(len_request);

        // for(int i=0; i<totLen; i++) {
        //     need_schedule[i] = valRequest(logical_clock, rq[i]);
        //     // need_schedule.push_back(valRequest(logical_clock, r));
        // }
        // sort(need_schedule.begin(), need_schedule.end());
        
        // while(need_schedule.size() && need_schedule.back().val < 0) {
        //     need_schedule.pop_back();
        // }

        // for(auto c : need_schedule) {
        //     cerr <<c.request.RequestID;
        //     cerr << " val = " <<c.val;
        //     cerr << " size=" << c.request.RequestSize << endl;
        // }
        // cerr << "above is need_schedule" << endl;
        // // 释放内存
        // delete []rq;
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

    // double solveGreedy(int *driver_volume, int *driver_capacity);

    Result *C_schedule(int logical_clock, Request *request_list, int len_request, Driver *driver_list, int len_driver)
    {
        cerr << "time: " << logical_clock << " schedule begin\n";

        int *driver_volume = new int[_driver_num];
        int *driver_capacity = new int[_driver_num];

        get_need_schedule(logical_clock, request_list, len_request);

        memset(driver_volume, 0, sizeof(int) * _driver_num);


        for (int i = 0; i < _driver_num; i++)
        {
            result[i].DriverID = driver_list[i].DriverID;
            result[i].LogicalClock = logical_clock;
            result[i].len_RequestList = 0;
            // cerr << "null = " << result[i].RequestList << " " << (void*)result[i].RequestList << '\n';
            if(result[i].RequestList != nullptr)
                delete []result[i].RequestList;
            result[i].RequestList = new int[need_schedule.size()];
            driver_capacity[i] = driver_list[i].Capacity;
        }

        // cerr << "ns size = " << need_schedule.size() << " get schedule end\n";

        vector<int>().swap(matchDriver);
        vector<int>().swap(finalMatchDriver);
        matchDriver.resize(need_schedule.size());
        finalMatchDriver.resize(need_schedule.size());

        // cerr << "begin solve\n";
        
        double bestAns = -1e9;
        for(int i=0; i<500; i++) {       
            double nowans = solveGreedy(driver_volume, driver_capacity);
            // cerr << "solve over\n";
            if(nowans > bestAns) {
                // cerr << "begin match\n";
                bestAns = nowans;
                
                // cerr << "match over\n";

                for(int j=0; j<matchDriver.size(); j++)
                    finalMatchDriver[j] = matchDriver[j];
            }
        }
 
        matchDriver2Result(result, finalMatchDriver, need_schedule);
        
        // cerr << "loop over\n";
        // cerr << "begin end\n";
        // cerr << "schedulsize = " << need_schedule.size() << " " << finalMatchDriver.size() << '\n';
        // 删除已经处理的请求
        for(int idx=0; idx < need_schedule.size(); idx++) {
            if(finalMatchDriver[idx] != -1) {
                // del
                need_schedule[idx].request.SLA = -100;
            }
        }

        // for(int i=0; i<_driver_num; i++) {
        //     cerr << "vol: " << driver_volume[i] << " cap: " << driver_capacity[i] << '\n';
        // }
        // for(int idx =0 ; idx < need_schedule.size(); idx ++ ) {
        //     if(finalMatchDriver[idx] == -1) {
        //         cerr << "id=" << need_schedule[idx].request.RequestID << " size = " << need_schedule[idx].request.RequestSize << " time: " << need_schedule[idx].timeout << " [" ;
        //         for(int k=0; k < need_schedule[idx].request.len_Driver; k ++ )
        //             cerr << need_schedule[idx].request.Driver[k] <<",";
        //         cerr << "]\n";
        //     }
        // }

        // cout <<(void*)(result) << " delete end\n";
        delete []driver_volume;
        delete []driver_capacity;

        return result;
    }

    

    const long long mod = 1e9+7;
    int randGen(int &base, int p) {
        base = 1ll * base * p % mod;
        return base;
    }

    double solveGreedy(int *driver_volume, int *driver_capacity) {
        random_device rd;
        int base = rd()%mod;
        int p = rd()%mod;
        for(int i=0; i < _driver_num; i++) 
            driver_volume[i] = 0;
        srand(time(NULL));
        double credits = 0;
        double deny = 1;
        // cout << "preproc\n";  
        for(int idx=0; idx < need_schedule.size(); idx++) {
            matchDriver[idx] = -1;
            valRequest &rq = need_schedule[idx];
            if(rq.request.len_Driver == 0) continue ;


            if(1.0* (randGen(base, p)%mod) / mod > deny) continue ;

            int bgDriver = randGen(base, p);

            for(int i=0; i<rq.request.len_Driver; i++) {
                int drID = rq.request.Driver[ (i + bgDriver) % rq.request.len_Driver ];
                if(driver_volume[drID] + rq.request.RequestSize <= driver_capacity[drID]) {

                    credits += rq.val * rq.request.RequestSize;
                    driver_volume[drID] += rq.request.RequestSize;
                    matchDriver[idx] = drID;
                    break;
                }
            }
            if(deny > 0.7) deny *= 0.99;
        }

        for(int idx=0; idx < need_schedule.size(); idx++) {
            if(matchDriver[idx] !=- 1) continue;
            valRequest &rq = need_schedule[idx];
            if(rq.request.len_Driver == 0) continue ;

            for(int i=0; i<rq.request.len_Driver; i++) {
                int drID = rq.request.Driver[i];
                if(driver_volume[drID] + rq.request.RequestSize <= driver_capacity[drID]) {
                    credits += rq.val * rq.request.RequestSize;
                    driver_volume[drID] += rq.request.RequestSize;
                    matchDriver[idx] = drID;
                    break;
                }
            }
        }
        // for(int idx=0; idx < need_schedule.size(); idx++) {
        //     valRequest &rq = need_schedule[idx];
        //     if(matchDriver[idx] != -1) {
        //         if(rq.request.RequestType == BE) credits += 
        //     }

        // }
        
        return credits;
    }
private:
    int _driver_num;
    vector<valRequest>need_schedule;
    vector<int>matchDriver; // -1表示不处理
    vector<int>finalMatchDriver; 
    Result* result;

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
    void C_gc(CScheduler* self){
        if(self == nullptr) {
            cerr << "CTMD\n";
        } else delete self;
    }
    void C_init(CScheduler *self, int driver_num) { return self->C_init(driver_num); }
    Result *C_schedule(CScheduler *self, int logical_clock, Request *request_list, int len_request, Driver *driver_list, int len_driver)
    {
        return self->C_schedule(logical_clock, request_list, len_request, driver_list, len_driver);
    }
}