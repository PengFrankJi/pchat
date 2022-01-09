CXX = g++
PREFIX = -Wall -std=c++11
SUFFIX = -pthread

TARGET = main
OBJS = my_request.h my_request.cpp locker.h threadpool.h main.cpp

main: $(OBJS)
	$(CXX) $(PREFIX) $(OBJS) -o $(TARGET) $(SUFFIX)

clean:
	rm $(TARGET)

