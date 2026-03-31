.PHONY: clean

CXX := g++-13
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -ggdb

SRC := bfc.cpp
EXE := bfc

$(EXE) : $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(EXE)

clean :
	rm -rf $(EXE)
