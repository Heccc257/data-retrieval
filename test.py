import json
import os
import re
import pandas as pd
import numpy  as np
import sys
import time
import argparse
from Scheduler import *

parser = argparse.ArgumentParser()
parser.add_argument('--schedule_func', type=str, default='FinalScheduler', help='The name of the schedule function')
parser.add_argument('--data_dir', type=str, default='data/', help='The path of the data')
parser.add_argument('--times',type=int,default=1)
cfg=parser.parse_args()



def get_json_data(file_path):
    pattern=re.compile(r'(.*)(\{.*\})',re.S)
    status_table={'driver_statues':[],'request_list':[]}
    
    with open(file_path, 'r') as f:
        for line in f.readlines():
            regx=pattern.search(line)
            head=regx.group(1)
            json_line=json.loads(regx.group(2))
            if head=="d":
                status_table['driver_statues'].append(json_line)
            else:
                status_table['request_list'].append(json_line)

    driver_statues_table=pd.DataFrame(status_table['driver_statues']).groupby('LogicalClock').apply(lambda x:x.to_dict(orient='records')).to_dict()
    # driver_num=max([len(table) for table in driver_statues_table.values()])
    driver_num=max([max([driver['DriverID'] for driver in table]) for table in driver_statues_table.values()])+1
    print(driver_num)
    request_list_table=pd.DataFrame(status_table['request_list']).groupby('LogicalClock').apply(lambda x:x.to_dict(orient='records')).to_dict()
    return driver_statues_table,request_list_table,driver_num


def simulate(driver_statues, request_list, driver_num,schedule_func):
    scheduler_test=schedule_func()
    scheduler_test.init(driver_num)
    schedule_result=[]
    time.sleep(1)
    
    test_time=time.time()
    for logical_clock,(driver,request) in enumerate(zip(driver_statues.values(),request_list.values())):
        # print("test.py", "logclock", logical_clock, "begin")
        start_time=time.time()
        result=scheduler_test.schedule(logical_clock,[json.dumps(single_request) for single_request in request],[json.dumps(single_driver) for single_driver in driver])
        # import pdb; pdb.set_trace()
        schedule_result.append([json.loads(single_result) for single_result in result])
        delta_time=time.time()-start_time

        # print("test.py", "logclock", logical_clock, "end schedule")

        assert delta_time<5, f"The time in scheduling logical_clock {logical_clock} is too long, please check your code."
    print(f"Total time is {time.time()-test_time}")
    # scheduler_test.deconstruct()
    return schedule_result


def validate(driver_statues, request_table, schedule_result):
    # validation
    for logical_clock,schedule_list in enumerate(schedule_result):
        driver_cur_statues={driver['DriverID']:driver for driver in driver_statues[logical_clock]}
        
        # import pdb;pdb.set_trace()
        data_requested=sum([sum([request_table[task]['RequestSize'] for task in schedule_device['RequestList']])>driver_cur_statues[schedule_device['DriverID']]['Capacity'] for schedule_device in schedule_list])
        # data_requested=sum([sum([request_table[task]['RequestSize'] for task in schedule_device['RequestList']])>driver_statues[199][schedule_device['DriverID']]['Capacity'] for schedule_device in schedule_list])
        # print(logical_clock)
        # if logical_clock==199:
        #     import pdb;pdb.set_trace()
        # logical_clock=199, Driver={'DriverID': 2, 'Capacity': 25200, 'LogicalClock': 199}, requests={'RequestID': 456961, 'RequestType': 'FE', 'SLA': 12, 'Driver': [2, 117, 163, 174, 185, 221, 257, 385, 389, 616, 649, 696, 702, 740, 762, 765, 775, 842, 976], 'RequestSize': 26051, 'LogicalClock': 183};{'RequestID': 495212, 'RequestType': 'EM', 'SLA': 1, 'Driver': [2, 18, 162, 281, 296, 317, 372, 469, 487, 504, 530, 549, 554, 673, 720, 733, 786, 806, 810, 815, 865, 941, 967, 993, 999], 'RequestSize': 14697, 'LogicalClock': 199}
        # 2 493868 495431  [30376, 11384] {'DriverID': 2, 'RequestList': [493868, 495431], 'LogicalClock': 199}
        # print(logical_clock)
        assert data_requested==0, f"In logical_clock {logical_clock} some request are invalisd to the constraint data, please check your code."
        
        # print(schedule_list)
        device_requested=sum([sum([(schedule_device['DriverID'] not in request_table[task]['Driver']) for task in schedule_device['RequestList']]) for i,schedule_device  in enumerate(schedule_list)])
        
        assert device_requested==0, "In logical_clock {} some request are invalisd to the constraint device, please check your code.".format(logical_clock)
        

def get_score(driver_statues, request_table, schedule_result):
    # score
    # TODO: 如果发送超过驱动器执行能力的请求，超出能力部分的请求将会被丢弃不进行处理。
    def calc_score(logical_clock,singel_request):
        delta_clock=singel_request['LogicalClock']-logical_clock+singel_request['SLA']
        request_size=singel_request['RequestSize']
        
        if singel_request['RequestType']=='FE' and delta_clock<=0 :
            # 前台取回超时罚分=min(12, 请求超时小时数)*?请求大小/50?
            return -min(12,-delta_clock)*np.ceil(request_size/50.0)
        elif singel_request['RequestType']=='BE' and delta_clock>=0:
            # 后台取回加分=0.5*?请求大小/50?
            return 0.5*np.ceil(request_size/50.0)
        elif singel_request['RequestType']=='EM' and delta_clock<=0:
            # 紧急取回超时罚分=2* min(12, 请求超时小时数)*?请求大小/50?
            return -2*min(12,-delta_clock)*np.ceil(request_size/50.0)
        else:
            return 0.0


    score=0.0
    schedule_table=sum([schedule for schedule in schedule_result],[])
    clock_device_of_excecute={request:schedule['LogicalClock'] for schedule in schedule_table for request in schedule['RequestList']}
    MAX_CLOCK=max(max(clock_device_of_excecute.values()),12)+1
    for singel_request in request_table:
        score+=calc_score(clock_device_of_excecute.get(singel_request['RequestID'],MAX_CLOCK),singel_request)
        
    return score


if __name__ == '__main__':
    os.system('make')
    scores={}
    data=[]
    if(cfg.data_dir=='data/'):
        data=['data/'+d for d in os.listdir(cfg.data_dir) if d[-1]=='g']
    else:
        data=[cfg.data_dir]
        
    scheduler= globals()[cfg.schedule_func]

    for data_dir in data:
        print("get data")
        driver_statues, request_list, driver_num=get_json_data(data_dir)
        request_table=sum([request for request in request_list.values()],[])
        driver_table=sum([driver for driver in driver_statues.values()],[])
        
        
        scores[data_dir.replace('data/','').replace('.log','')]=[]
        for i in range(cfg.times):
            print("simulating")
            schedule_result=simulate(driver_statues, request_list, driver_num,scheduler)
            print("validating")
            validate(driver_statues, request_table, schedule_result)
            print("scoring")
            score=get_score(driver_statues, request_table, schedule_result)
            print(f"Your score is {score}")
            scores[data_dir.replace('data/','').replace('.log','')].append(score)
    for k,v in scores.items():
        print(f"{k} :\t {v}")
