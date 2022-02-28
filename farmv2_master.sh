load=none

for((i=0;i<1;i++))
do
numactl --membind 0 --cpunodebind 0 ./farmv2_p $i ${load} M
sleep 2
done

