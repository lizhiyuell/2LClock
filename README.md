# 2LClock
## Introduction
This code is the prototype of *2LClock*, which utilizes fast RDMA network to provide high accuracy clock among distributed servers.
2LClock is part of the distributed in-memory transaction system **Aurogon** published on [**USENIX FAST'22**](https://www.usenix.org/conference/fast22/presentation/jiang).
This repository contains the source code of 2LClock and a number of test scripts.
The version integrated in Aurogon is available [here](https://github.com/THU-jty/Aurogon).


## Repository structure
- global.h: global Parameters definition.

- ifconfig: IP configuration of involved machines.

- clock_sync.h/cpp: Clock synchronization algorithm.

- svm.h/cpp: C++ version of support vector machine(SVM), adpoted from [libsvm](https://github.com/cjlin1/libsvm).

- rc_ts.h/cpp: RDMA RPC for probing, using *Reliable Connection*(RC).

- ud_ts.h/cpp: RDMA RPC for probing, using *Unreliable Datagram*(UD).

- rdma_ts.h/cpp: Simple RDMA RPC for communication.

- GT_center.cpp: Clock coordinator, which only runs in initializing phase.

- GT_master.cpp: Example codes on how the clock synchronization APIs work.

- clock_failure.h/cp: Failure handling realization.

- FR_center.cpp/FR_test.cpp/FR_root.cpp: Failure recovery scripts.

- frconfig_normal/frconfig_backup: IP configuration when testing failure recovery.

- directory data: Runtime output for probe data (only effective when "OUTPUT_MODE" in **global.h** is on) as well as accuracy results.

## Compile
make

## Module test
This part gives the example of how to use the 2LClock API.
Before running the script, you should modify the **number of servers and the IP list in ifconfig**.
In 2LClock, the servers are organized with K-ary tree, where K is determined by the *TREE_DEGREE_MAX* in **global.h**.
The servers in IP list are put sequentially in each layer of the K-ary tree.
For each server, run the "\${code_path}/gt_master \$id" in each server.
The variable \$id is the index of the relevant IP address in **ifconfig**.

For example, assuming you run 2LClock on 4 servers and set *TREE_DEGREE_MAX=2*.
You should modify the **ifconfig** like:

```
4
172.23.12.124
172.23.12.125
172.23.12.128
172.23.12.131
```
In each of the four servers, run the following commands respectively. The variable ${code_path} stands for the path of 2LClock repository.
```
@172.23.12.124: ${code_path}/gt_master 0
@172.23.12.125: ${code_path}/gt_master 1
@172.23.12.128: ${code_path}/gt_master 2
@172.23.12.131: ${code_path}/gt_master 3
```
After these commands, 2LClock will build a network topology of:

&ensp;124

/&emsp;&emsp;\

125&ensp;128

|

131

## Accuracy test
### 2LClock and HUYGENS
Modify the \$parent_ip and \$code_path in 2LClock_test.sh/huygens_test.sh.
Then change the ifconfig as:
```
3
$parent_ip
$this_ip
$this_ip
```
Where $this_ip is the IP address of the server running the above test codes.

The result is available in **${code_path}/data**.

### farmv2-clock
The test runs in two servers: master and slave server.
Modify the variable \${master_ip} in **farmv2_slave.cpp** before running.
Then run \${code_path}/farmv2_slave.sh on slave server and \${code_path}/farmv2_master.sh on master server.

## Failure recovery
This part tests the failure recovery algorithm of 2LClock by implementing a case specific test. 
This case tests how two servers (*server C&D*) shift to a backup server(*server B*) when their common father server (*server A*) fails.
At first the network topology is:

&emsp;&emsp;A

/&emsp;&emsp;|&emsp;&emsp;\

B&emsp;&emsp;C&emsp;&emsp;D

After *server A* fails, the network topogy changes to:

&emsp;D

/&emsp;&emsp;\

B&emsp;&emsp;C

To run the test, there are four related scripts:
```
*server A*: ${code_path}/gt_master 0
*server B&C*: numactl --membind 0 --cpunodebind 0 ${code_path}/fr_test $ip_of_D
*server D*: ${code_path}/fr_root
any server: ${code_path}/fr_center
```
Then change the IP list of two files:

*frconfig_normal*:
```
4
ip_of_A
ip_of_B
ip_of_C
ip_of_D
```

*frconfig_backup*:
```
3
ip_of_D
ip_of_B
ip_of_C
```

To get the plot, run "cd \${code_path}/data" and "python3 failure_analysis.py".


## Break the test
Run **killall.sh** at all involved servers.
We recommend to run this script when any of the programs does not *exit* normaly.