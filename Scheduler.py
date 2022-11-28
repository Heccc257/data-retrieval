import time
from abc import ABCMeta, abstractmethod
import numpy as np
import json
from ctypes import *





class Scheduler(metaclass=ABCMeta):
    @abstractmethod
    def init(self, driver_num: int) -> None:
        pass

    @abstractmethod
    def schedule(self, logical_clock: int, request_list: list, driver_statues: list) -> list:
        pass


class DemoScheduler(Scheduler):
    def __init__(self):
        pass

    def init(self, driver_num: int):
        self.driver_num = driver_num
        return

    def schedule(self, logical_clock: int, request_list: list, driver_statues: list) -> list:
        request_list=[json.loads(request) for request in request_list]
        driver_statues=[json.loads(driver) for driver in driver_statues]
        driver_capacity=np.array([driver_statues[i]['Capacity'] for i in range(self.driver_num)])
        driver_volume=np.zeros(self.driver_num)
        schedule_result=[{'DriverID':i,'RequestList':[],'LogicalClock':logical_clock} for i in range(self.driver_num)]
        
        for request in request_list:
            for device in request['Driver']:
                if driver_volume[device]+request['RequestSize']<=driver_capacity[device]:
                    driver_volume[device]+=request['RequestSize']
                    schedule_result[device]['RequestList'].append(request['RequestID'])
                break
        schedule_result=[json.dumps(result) for result in schedule_result]
        return schedule_result
    
class FinalScheduler(Scheduler):
    
    class Request(Structure):
        _fields_ = [("RequestID", c_int),
                    ("RequestType", c_int),# FE:0, BE:1, EM:2
                    ("RequestSize", c_int),
                    ("LogicalClock", c_int),
                    ("SLA", c_int),
                    ("len_Driver", c_int),
                    ("Driver", POINTER(c_int))]
        
    class Driver(Structure):
        _fields_ = [("DriverID", c_int),
                    ("Capacity", c_int),
                    ("LogicalClock", c_int)]
        
        
    class Result(Structure):
        _fields_ = [("DriverID", c_int),
                    ("LogicalClock", c_int),
                    ("len_RequestList", c_int),
                    ("RequestList", POINTER(c_int))]


        
    def __init__(self):
        self.lib = cdll.LoadLibrary('./myscheduler.so')
        
        self.lib.C_new.argtypes = []
        self.lib.C_new.restype = c_void_p
        
        self.lib.C_init.argtypes = [c_void_p, c_int]
        self.lib.C_init.restype = c_void_p

        self.lib.C_schedule.argtypes = [c_void_p,c_int, POINTER(self.Request),c_int, POINTER(self.Driver),c_int]
        self.lib.C_schedule.restype = POINTER(self.Result)

        self.lib.C_gc.argtypes=[c_void_p]
        self.lib.C_gc.restype=c_void_p

        self.obj = self.lib.C_new()

    def init(self, driver_num: int):
        self.driver_num = driver_num
        self.lib.C_init(self.obj, driver_num)
        return

    def schedule(self, logical_clock: int, request_list: list, driver_statues: list) -> list:
        request_list=[json.loads(request) for request in request_list]
        driver_statues=[json.loads(driver) for driver in driver_statues]
        
        # import pdb;pdb.set_trace()
        C_request_list=[]
        C_driver_statues=[]
        for request in request_list:
            C_request_list.append(self.Request(request['RequestID'],0 if request['RequestType']=='FE' else 1 if request['RequestType']=='BE' else 2,request['RequestSize'],request['LogicalClock'],request['SLA'],len(request['Driver']),(c_int*len(request['Driver']))(*request['Driver'])))
        for driver in driver_statues:
            C_driver_statues.append(self.Driver(driver['DriverID'],driver['Capacity'],driver['LogicalClock']))

        C_schedule_result=self.lib.C_schedule(self.obj,logical_clock,(self.Request * len(C_request_list))(*C_request_list),len(C_request_list),(self.Driver * len(C_driver_statues))(*C_driver_statues),len(C_driver_statues))
        
        Results=[]
        for i in range(self.driver_num):
            RequestList=[]
            for j in range(C_schedule_result[i].len_RequestList):
                RequestList.append(C_schedule_result[i].RequestList[j])
            Results.append(json.dumps({"DriverID":C_schedule_result[i].DriverID,"RequestList":RequestList,"LogicalClock":C_schedule_result[i].LogicalClock}))
        return Results

    def deconstruct(self):
        print("decon")
        self.lib.C_gc(self.obj)
        print("decon ok")