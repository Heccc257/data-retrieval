# data-retrieval

## 评测注意事项！！！
### `scheduler.py`会调用`build/myFinalScheduler.so`。所以请**务必**将`scheduler.py`与`build`文件夹放在同一目录下，或者直接将**完整工程**直接一同拷贝。

<center/> FishCatcher </center>

| files            | description                                                  |
| ---------------- | ------------------------------------------------------------ |
| `scheduler.py`   | scheduler接口，包含class FinalScheduler FinalScheduler`                    |
| `myscheduler.so` | `scheduler.cpp`编译生成的动态链接库，在`build/`目录下，被FinalScheduler调用 |
| `scheduler.cpp`  | scheduler算法源码实现                                       |
| `新文档 78.pdf`  | 原创声明                                                     |
| 其它文件	         | 辅助工具以及test bench                                       |


### TODO