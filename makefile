CC=gcc

daemon: daemon.cpp
	$(CC) -o daemon daemon.cpp

clean:
	rm -f daemon
	rm -f *.o
