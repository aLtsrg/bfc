.PHONY: clean

CXX := c++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -ggdb

SRC := bf.cpp
EXE := bf

bf : bf.cpp
	$(CXX) $(CXXFLAGS) $(SRC) -o $(EXE)

clean :
	rm -rf $(EXE)
