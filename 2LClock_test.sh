load=none
parent_ip=172.23.12.125
code_path=/root/lzy/nfs_file/clock_sync/

for((i=0;i<1;i++))
do
echo test$i
./gt_center &
ssh root@${parent_ip} ${code_path}"gt_master 0" &
numactl --membind 0 --cpunodebind 0 ${code_path}/2LClock_p $i $load
sleep 2

done
