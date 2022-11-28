# data-retrieval

<center/> FishCatcher队</center>

| files            | description                                                  |
| ---------------- | ------------------------------------------------------------ |
| `scheduler.py`   | scheduler接口，包含`class FinalScheduler`                    |
| `scheduler.cpp`  | scheduler算法源码实现                                        |
| `myscheduler.so` | `scheduler.cpp`通过`Makefile`生成的动态链接库，被`FinalScheduler`调用 |
| `Makefile`       | 用`scheduler.cpp`生成动态链接库`myscheduler.so`              |
| `新文档 78.pdf`  | 原创声明                                                     |
| 其它文件         | 辅助工具以及test bench                                       |

***测试时应将`myscheduler.so`放到`scheduler.py`同文件夹下***



### TODO