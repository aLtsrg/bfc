.PHONY: clean, debug

CXX := g++-13
CXXFLAGS := -std=c++20 -O3 -Wall -Wextra -Wpedantic -Wconversion
DEBUGFLAGS := -std=c++20 -ggdb3 -O0 -Wall -Wextra -Wpedantic -Wconversion \
			  -fsanitize=address,undefined -fno-omit-frame-pointer

SRC := bfc.cpp
EXE := bfc

$(EXE) : $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(EXE)

debug : $(SRC)
	$(CXX) $(DEBUGFLAGS) $(SRC) -o $(EXE)

clean :
	rm -rf $(EXE)
