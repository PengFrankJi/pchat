CXX = g++
CFLAGS = -Wall -std=c++11

run: main.o locker.o threadpool.o my_request.o sockfd.o
	$(CXX) $(CFLAGS) -o main.o locker.o threadpool.o my_request.o sockfd.o -pthread

main.o: main.cpp locker.h threadpool.h my_request.h my_request.cpp sockfd.h
	$(CXX) $(CFLAGS)  main.cpp locker.h threadpool.h my_request.h my_request.cpp sockfd.h

locker.o: locker.h
	$(CXX) $(CFLAGS) locker.h

threadpool.o: threadpool.h locker.h
	$(CXX) $(CFLAGS) threadpool.h locker.h

my_request.o: my_request.h my_request.cpp locker.h
	$(CXX) $(CFLAGS) my_request.h my_request.cpp locker.h
	
sockfd.o: sockfd.h
	$(CXX) $(CFLAGS) sockfd.h
	
clean:
	rm main *.o

