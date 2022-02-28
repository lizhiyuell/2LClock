load=none
master_ip=172.23.12.125

for((i=0;i<1;i++))
do
numactl --membind 0 --cpunodebind 0 ./farmv2_p $i ${load} "S" ${master_ip}
sleep 2
done
