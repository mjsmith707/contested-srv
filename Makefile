CC=g++
CFLAGS=-std=c++11 -g -c -Wall
LDFLAGS= -lmysqlcppconn -lpthread -lboost_system -lboost_thread -ljsoncpp
SOURCES=./source/srv_main.cpp ./source/srv_db.cpp ./source/config.cpp ./source/logger.cpp ./source/server.cpp ./source/request_parser.cpp ./source/request_handler.cpp ./source/reply.cpp ./source/mime_types.cpp ./source/connection.cpp ./source/srv_randcontst.cpp ./source/srv_topcontst.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=./bin/Debug/contested-srv

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	mkdir -p ./bin/
	mkdir -p ./bin/Debug/
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJECTS)
	rm -rf $(EXECUTABLE)
