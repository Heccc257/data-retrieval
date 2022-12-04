import os
import numpy as np
from optparse import OptionParser
parser = OptionParser()
parser.add_option('--num_drivers',type="int",default=1000) # （最大）磁带驱动器数量
parser.add_option('--max_requests',type="int",default=5000) # 最大请求数量
parser.add_option('--max_bandwidth',type="int",default=1000) # 每个磁带驱动器最大负载（x100)
parser.add_option('--rqs2dr_bwratio',type="float",default=1.5) # 每小时请求总数据量与总负载量之比（由于每个请求都必须可以被磁带驱动器满足，实际比值要小于此值）
parser.add_option('--output_file',type="string",default="./") # 输出文件夹
parser.add_option('--schedule_time',type="int",default=200)
parser.add_option('--default',action="store_true",dest="default",default=False)
(options, args) = parser.parse_args()

def trunc_nomal(loc,scale,trunc_l=-1,trunc_h=-1):
    if trunc_l==-1:
        trunc_l=loc-3*scale
    if trunc_h==-1:
        trunc_h=loc+3*scale
    return max(trunc_l,min(trunc_h,np.random.normal(loc,scale)))
def str_dr(id,cap,clk):
    return "d{\"DriverID\": "+str(id)+", \"Capacity\": "+str(cap)+", \"LogicalClock\": "+str(clk)+"}\n"
def str_rqs(id,typ,dr_list,size,clk):
    SLA={0:("FE","1"),1:("FE","6"),2:("FE","12"),3:("BE","12"),4:("EM","1")}
    return "r"+str(clk).rjust(8,"0")+"{\"RequestID\": "+str(id)+", \"RequestType\": \""+SLA[typ][0]+"\", \"SLA\": "+SLA[typ][1]+", \"Driver\": "+str(dr_list)+", \"RequestSize\": "+str(size)+", \"LogicalClock\": "+str(clk)+"}\n"
def generate_data(file,num_dr,num_t,max_rqs,max_bw,bw_ratio):
    sum_rqs=0
    for clk in range(num_t):
        # Generate drivers data
        bw_dr=[]
        for id_dr in range(num_dr):
            bw=int(trunc_nomal(max_bw/2,max_bw/3,0,max_bw))*100
            if bw<max_bw/8:
                bw=0
            bw_dr.append(bw)
            if bw!=0:
                file.write(str_dr(id_dr,bw,clk))
        sum_bw=sum(bw_dr)
        num_rqs=int(trunc_nomal(max_rqs/2,max_rqs/6,1,max_rqs))

        # Generate requests data
        bw_rqs=[]
        for id_rqs in range(num_rqs):
            bw=trunc_nomal(5,1,0,10)
            bw_rqs.append(bw)
        sum_rqs_bw=sum(bw_rqs)
        for id_rqs in range(num_rqs):
            dr_list=[]
            dr_num=int(min(1+abs(np.random.standard_normal())*(1+num_dr/100),num_dr))
            for d in range(dr_num):
                dr_list.append(np.random.randint(0,num_dr))
            dr_list=list(set(dr_list))
            dr_list.sort()
            bandwith=min(bw_dr[dr_list[np.random.randint(len(dr_list))]],int(bw_rqs[id_rqs]/sum_rqs_bw*bw_ratio*sum_bw))
            bandwith=bandwith-bandwith/10*abs(np.random.standard_normal())
            file.write(str_rqs(sum_rqs+id_rqs,np.random.randint(5),dr_list,int(bandwith),clk))
        sum_rqs=sum_rqs+num_rqs
            
        if(clk%50==0):
            print("schedule time: ",clk)
    
    # Generate drivers data
    bw_dr=[]
    for id_dr in range(num_dr):
        bw=int(trunc_nomal(max_bw/2,max_bw/6,1,max_bw))*100
        bw_dr.append(bw)
        file.write(str_dr(id_dr,bw,clk+1))
    sum_bw=sum(bw_dr)
    num_rqs=int(trunc_nomal(max_rqs/2,max_rqs/6,1,max_rqs))


if options.default:
    num_dr=np.random.randint(1,1000)
    num_t=np.random.randint(10,1000)
    max_rqs=np.random.randint(10,3000)
    max_bw=np.random.randint(1,1e3)
    bw_ratio=np.random.random()/2+1
else:
    num_dr=options.num_drivers
    num_t=options.schedule_time
    max_rqs=options.max_requests
    max_bw=options.max_bandwidth
    bw_ratio=options.rqs2dr_bwratio
opath=options.output_file+"/data_dr"+str(num_dr)+"_h"+str(num_t)+"_rqs"+str(max_rqs)+"_bw"+str(max_bw)+"_bwratio"+str(bw_ratio)+".log"
f=open(opath,"w+",encoding='UTF-8')
print("""Generating... """+opath+"""
    --max number of driver: """+str(num_dr)+""",
    --number of hours: """+str(num_t)+""",
    --max number of requests per hour: """+str(max_rqs)+""",
    --max bandwidth each driver can handle per hour: """+str(max_bw)+""",
    --request bandwidth:driver bandwidth per hour: """+str(bw_ratio))
generate_data(f,num_dr,num_t,max_rqs,max_bw,bw_ratio)
f.close()
