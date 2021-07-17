CXX = g++

CPPFLAGS += -MMD -MP
CXXFLAGS += -g -Wall -Werror -O3 -std=c++17 -pthread
LDFLAGS += -pthread

SRCS = $(wildcard src/*.cpp)

all: recommender

-include $(SRCS:%.cpp=%.d)

recommender: $(SRCS:%.cpp=%.o)
	$(CXX) $(LDFLAGS) -o $@ $^

clean:
	rm -rf src/*.d
	rm -rf src/*.o
	rm -rf recommender
