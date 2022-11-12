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
    int RequestType;
    int RequestSize;
    int LogicalClock;
    int SLA;
    int len_Driver;
    int *Driver;
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
    int _driver_num;

public:
    CScheduler() {}
    ~CScheduler() {}
    void C_init(int driver_num)
    {
        _driver_num = driver_num;
    }
    Result *C_schedule(int logical_clock, Request *request_list, int len_request, Driver *driver_list, int len_driver)
    {
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

        My_algo(logical_clock, request_list, len_request, driver_list, len_driver, result, driver_volume, driver_capacity);

        return result;
    }

private:
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