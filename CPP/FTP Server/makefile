CXX      = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -pthread -g

TARGET = my_ftp

all: $(TARGET)

$(TARGET): my_ftp.cpp
	$(CXX) $(CXXFLAGS) -o $@ $

clean:
	rm -f $(TARGET)