CXX = g++482
CXXFLAGS = --std=c++11 -Wall -O0 
LDFLAGS = -Wl,-rpath=/usr/local/gcc482/lib64 -lpthread -lsctp

CXXFILES = src/main.cpp src/Message.cpp src/parse_config.cpp src/run_node.cpp
OBJFILES = $(CXXFILES:%.cpp=%.o)

all: bin/CS6380Project1

bin/CS6380Project1: $(OBJFILES)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f src/*.o bin/CS6380Project1
	
.c.o:
	$(CXX) -c $<

src/main.o: src/main.cpp src/Message.h src/CS6380Project1.h
src/Message.o: src/Message.cpp src/Message.h
src/parse_config.o: src/parse_config.cpp src/Message.h src/CS6380Project1.h
src/run_node.o: src/run_node.cpp src/Message.h src/CS6380Project1.h
