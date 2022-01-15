CXX = g++
PREFIX = -Wall -std=c++11 -g
SUFFIX = -pthread -lmysqlclient

TARGET = main
OBJS = my_request.h my_request.cpp locker.h threadpool.h \
	   server.h server.cpp main.cpp\
	   pool/sqlconnpool.h pool/sqlconnpool.cpp pool/sqlconnRAII.h \
	   log/log.h log/log.cpp\
	   buffer/buffer.h buffer/buffer.cpp

main: $(OBJS)
	$(CXX) $(PREFIX) $(OBJS) -o $(TARGET) $(SUFFIX)

clean:
	rm $(TARGET)

