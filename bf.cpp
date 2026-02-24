//#define NDEBUG

#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <cassert>

enum class Op {
    Undefined, 
    AddCell, 
    SubCell, 
    AddPtr,
    SubPtr,
    LoopStart,
    LoopEnd,
    Output,
    Input, 
};

struct IREntry {
    Op op {};
    int arg {};
};

// read input line by line
// could put other information here like line number etc.
// TODO: can this error? if so make it bool and return if error.
void readSourceFile(std::ifstream& bfFile, std::string& outputString)
{
    for (std::string line; std::getline(bfFile >> std::ws, line);)
        outputString += line + "\n";
}

std::string filter(std::string_view contents)
{
    std::string result{};
    for (const auto& c : contents){
        if ( c == '+' || c == '-'|| c == '>' || c == '<' 
                || c == '[' || c == ']' || c == '.' || c == ',' )
            result += c;
    }
    return result;
}


bool validateAndMapBrackets(std::string_view code, std::unordered_map<int, int>& idxToLpNbr)
{
    std::vector<int> stack{};
    int idx {};
    int loopNumber {1};

    for (const auto& c : code){
        if (c == '['){
            stack.push_back(loopNumber);
            idxToLpNbr[idx] = loopNumber;
            ++loopNumber;
        } else if (c == ']'){
            if (stack.size() ==  0) return false;
            idxToLpNbr[idx] = stack.back();
            stack.pop_back();
        }
        ++idx;
    }

    if (stack.size() > 0) return false;

    return true;
}

void printMap(std::unordered_map<int, int> map)
{
    std::cout << "[\n";
    for(const auto& entry : map){
        std::cout << "  ENTRY: "<< entry.first << ',' << entry.second << '\n';
    }
    std::cout << "]\n";
}

// TODO: maybe bool?
void buildIR(std::vector<IREntry>& IR, std::unordered_map<int, int>& idxToLpNbr, std::string_view code)
{
    int idx{};
    for ( const auto& c : code ){
        switch (c)
        {
        case '+':
            IR.push_back({Op::AddCell, 1});
            break;
        case '-':
            IR.push_back({Op::SubCell, 1});
            break;
        case '>':
            IR.push_back({Op::AddPtr, 1});
            break;
        case '<':
            IR.push_back({Op::SubPtr, 1});
            break;
        case '[':
            assert(idxToLpNbr.contains(idx) && "bracket idx not in map");
            IR.push_back({Op::LoopStart, idxToLpNbr[idx]});
            break;
        case ']':
            assert(idxToLpNbr.contains(idx) && "bracket idx not in map");
            IR.push_back({Op::LoopEnd, idxToLpNbr[idx]});
            break;
        case '.':
            IR.push_back({Op::Output, 0});
            break;
        case ',':
            IR.push_back({Op::Input, 0});
            break;
        default:
            std::cerr << "\033[1;31mERROR: \033[0m" << "unreachable state when building IR\n";
            break;
        }
        ++idx;
    }
}

void printIR(std::vector<IREntry>& IR)
{
    int idx {};
    std::string opString {};
    std::cout << "[\n";
    for(const auto& entry : IR){
        
        if (entry.op == Op::Undefined) opString = "Undefined";
        if (entry.op == Op::AddCell) opString = "AddCell";
        if (entry.op == Op::SubCell) opString = "SubCell";
        if (entry.op == Op::AddPtr) opString = "AddPtr";
        if (entry.op == Op::SubPtr) opString = "SubPtr";
        if (entry.op == Op::LoopStart) opString = "LoopStart";
        if (entry.op == Op::LoopEnd) opString = "LoopEnd";
        if (entry.op == Op::Output) opString = "Output";
        if (entry.op == Op::Input) opString = "Input";
        
        std::cout << idx << ": " << opString << ',' << entry.arg << '\n';
        ++idx;
    }
    std::cout << "]\n";
}

int main(int argc, char *argv[])
{
    // TODO: check for too many arguments
    // TODO: add --help flag, and -o output
    if (argc < 2){
        std::cerr << "\033[1;31mERROR: \033[0m" << "No file provided\n";
        return 1;
    }

    std::string input;
    std::string fileName{ argv[1] };
    std::ifstream bfFile{ fileName };

    if (!bfFile || !fileName.ends_with(".bf")){
        std::cerr << "\033[1;31mERROR: \033[0m" << "File called \033[1;33m`" 
            << argv[1] << "`\033[0m was not found or is of incorrect type\n";
        return 1;
    }
    
    readSourceFile(bfFile, input); 
    std::cout << "RAW INPUT: \n" << input << '\n';

    input = filter(input); 
    std::cout << "FILTERED: \n" << input << '\n';

    std::unordered_map<int, int> indexToLoopNumber{};
    if (!validateAndMapBrackets(input, indexToLoopNumber)){
        std::cerr << "\033[1;31mSYNTAX ERROR: \033[0m" << "brackets not balanced\n";
        return 0;
    }

    std::vector<IREntry> IR{};
    buildIR(IR, indexToLoopNumber, input);
    
    printIR(IR);


    // TODO: build asm from IR

    return 0;
}
