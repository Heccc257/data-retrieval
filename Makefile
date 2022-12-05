all:
	g++ --shared scheduler.cpp anneal.cpp greedy.cpp scheduler.h -o myscheduler.so -fPIC -O3
