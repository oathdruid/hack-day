CXX = /opt/compiler/gcc-10/bin/g++

CPPFLAGS += -MMD -MP
CXXFLAGS += -g -Wall -Werror -O3 -std=c++17 -pthread
LDFLAGS += -pthread

SRCS = $(wildcard src/*.cpp)

all: recommander

-include $(SRCS:%.cpp=%.d)

recommander: $(SRCS:%.cpp=%.o)
	$(CXX) $(LDFLAGS) -o $@ $^

clean:
	rm -rf src/*.d
	rm -rf src/*.o
	rm -rf recommander
