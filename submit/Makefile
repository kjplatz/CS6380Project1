CXX = g++482
CXXFLAGS = --std=c++11 -Wall -O0 -g
LDFLAGS = -Wl,-rpath=/usr/local/gcc482/lib64 -lpthread -lsctp

CXXFILES = src/main.cpp src/Message.cpp src/parse_config.cpp src/Node.cpp
OBJFILES = $(CXXFILES:%.cpp=%.o)

all: bin/CS6380Project2

bin/CS6380Project2: $(OBJFILES)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f src/*.o bin/CS6380Project2
	
.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

src/main.o: src/main.cpp src/Message.h src/CS6380Project2.h
src/Message.o: src/Message.cpp src/Message.h
src/parse_config.o: src/parse_config.cpp src/Message.h src/CS6380Project2.h
src/Node.o: src/Node.cpp src/Message.h src/CS6380Project2.h
