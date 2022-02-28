CFLAGS := -g -Wall -std=c++11
CC      := g++
LIBS 	:= -libverbs -lpthread -lmlx5
LDFLAGS := $(LDFLAGS) $(LIBS)
APPS    := fr_center fr_root fr_test gt_master gt_center 2LClock_p huygens_p farmv2_p 
.PHONY:clean

all: $(APPS)

source := clock_sync.cpp rdma_ts.cpp rc_ts.cpp ud_ts.cpp global.h svm.cpp clock_failure.cpp 

gt_center: $(source) GT_center.cpp
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

gt_master: $(source) GT_master.cpp
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

fr_test: $(source) FR_test.cpp
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

fr_center: $(source) FR_center.cpp
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

fr_root: $(source) FR_root.cpp
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

2LClock_p: $(source) 2LClock_p.cpp
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

farmv2_p : $(source) farmv2_p.cpp
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

huygens_p: $(source) huygens_p.cpp
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(APPS) *.o
