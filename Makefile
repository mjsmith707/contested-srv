CC=g++
CFLAGS=-std=c++11 -g -c -Wall
LDFLAGS= -lmysqlcppconn
SOURCES=./source/srv_main.cpp ./source/srv_db.cpp ./source/config.cpp ./source/logger.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=./bin/Debug/contested-srv

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJECTS)
	rm -rf $(EXECUTABLE)
