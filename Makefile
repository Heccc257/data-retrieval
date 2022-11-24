all:
	g++ --shared scheduler.cpp -o myscheduler.so -fPIC -O3

clean:
	rm myscheduler.so