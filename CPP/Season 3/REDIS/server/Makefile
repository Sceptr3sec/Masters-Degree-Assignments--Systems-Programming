CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread
TARGET = redis_server
SRCS = main.cpp resp.cpp connection.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
