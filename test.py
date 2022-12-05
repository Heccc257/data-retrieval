import json
import os
import re
import pandas as pd
import numpy  as np
import sys
import time
import argparse


parser = argparse.ArgumentParser()
parser.add_argument('--scheduler', type=str, default='FinalScheduler', help='The name of the schedule function')
parser.add_argument('--data_dir', type=str, default='data/Demo.log', help='The path of the data')
parser.add_argument('--times',type=int,default=1)
parser.add_argument('--submit',action='store_true')
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
    scheduler_test=schedule_func
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
        
        data_requested=sum([sum([request_table[task]['RequestSize'] for task in schedule_device['RequestList']])>driver_cur_statues[schedule_device['DriverID']]['Capacity'] for schedule_device in schedule_list])

        assert data_requested==0, f"In logical_clock {logical_clock} some request are invalisd to the constraint data, please check your code."
        
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
    if os.path.exists('build'):
        os.system('rm -rf build')
    os.makedirs('build',exist_ok=True)
    if cfg.submit:
        from Scheduler import *
        schedulers = ['FinalScheduler']
    else:
        from SchedulerForDev import *
        schedulers=cfg.scheduler.split(',')

    appendSrc = " src/anneal.cpp src/greedy.cpp src/scheduler.h "
    for s in schedulers:
        os.system(f'g++ --shared src/{s}.cpp {appendSrc} -o build/my{s}.so -fPIC -O3')
    scores={s:{} for s in schedulers}
    
    # make handout
    if os.path.exists('handout') == False:
        os.makedirs('handout',exist_ok=True)
    handoutList = " Scheduler.py please\ README\ first.md build "
    os.system(f'cp -rf {handoutList} handout')
    os.system('touch handout/请一定看看\ please\ REDAME\ first.md!!!')
    
    data=[]
    if(cfg.data_dir=='data/'):
        data=['data/'+d for d in os.listdir(cfg.data_dir) if d[-1]=='g']
    else:
        data=cfg.data_dir.split(',')

    for data_dir in data:
        print("get data")
        driver_statues, request_list, driver_num=get_json_data(data_dir)
        request_table=sum([request for request in request_list.values()],[])
        driver_table=sum([driver for driver in driver_statues.values()],[])
        
        for s in schedulers:
            if cfg.submit:
                scheduler= FinalScheduler()
            else:
                scheduler= FinalScheduler(s)
                
            scores[s][data_dir.replace('data/','').replace('.log','')]=[]
            for i in range(cfg.times):
                print("simulating")
                schedule_result=simulate(driver_statues, request_list, driver_num,scheduler)
                print("validating")
                validate(driver_statues, request_table, schedule_result)
                print("scoring")
                score=get_score(driver_statues, request_table, schedule_result)
                print(f"Your score is {score}")
                scores[s][data_dir.replace('data/','').replace('.log','')].append(score)
    print('\n***************************')
    print('you are in SUBMIT mode' if cfg.submit else 'you are in DEVELOP mode')
    print('***************************\n')
    for k,v in scores.items():
        print(f"{k}: ")
        for v,d in scores[k].items():
            print(f"\t{v}:\n\t\t{d}")
