# 

# CPU负载均衡的TSP并行算法

这是基于pthread实现的CPU负载均衡的TSP并行算法，是2021学年ECNU并行计算课程的大作业。

## 算法思想

1. bfs划分任务

   通过bfs进行任务划分，将原先的单个dfs任务转化成与核数相等的多个dfs任务，充分利用核资源

2. dfs过程中监控线程个数，活跃线程数减少之后根据条件判断是否拆分并新建任务。拆分的方式是从dfs自己的栈中抽取栈底的元素作为新线程dfs的起点，进行dfs。这里选取栈底元素主要是因为，栈底元素已经行走的路径最短，因此剩余计算量最大，由此新建出的线程生命周期较长；此外，栈底元素是一个新的dfs的根结点，抽取这个节点不会有副作用。

## 文件结构

1. main.cpp 算法主文件
2. input.txt 测试用例
3. test.js 随机生成一个测试用例

