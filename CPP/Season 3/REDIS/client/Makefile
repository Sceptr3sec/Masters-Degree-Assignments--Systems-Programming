CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET = main
SRCS = main.cpp resp.cpp connection.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)
