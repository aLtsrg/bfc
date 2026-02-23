.PHONY: clean

CXX := c++
CXXFLAGS := -Wall -Wextra -Wpedantic -Wconversion -ggdb

SRC := bf.cpp
EXE := bf

bf : bf.cpp
	$(CXX) $(CXXFLAGS) $(SRC) -o $(EXE)

clean :
	rm -rf $(EXE)
