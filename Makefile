CXX = g++
CXXFLAGS = -std=c++17 -O2

code: main.cpp
	$(CXX) $(CXXFLAGS) -o code main.cpp

clean:
	rm -f code *.dat

.PHONY: clean

