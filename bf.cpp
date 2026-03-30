//#define NDEBUG

#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <cassert>
#include <format>



//TODO: make more robust compand line argument system
//          - learn how to use flags correctly
//          - add -help
//          - add -ir and -s for outputing steps
//          - safely add -o
//          - find replacement for std::system
//      start implementing optimizations
//          - constant folding 
//          - cell reset detection [+], [-]
//          - others

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

//TODO: works for some programs, test on more
//      add pretty printing for the asm
void translate(const std::vector<IREntry>& IR)
{
    //counter for label generation
    int counter{};

    // format example:
    //     out << std::format("Hello {}\n", 69);
    
    // function should:
    // go through IR generating asm per IREntry
    // pass program name as argument -- only if not using system, otherwise its dangerous
    
    // header
    std::ofstream out("a.s");
    out << "bits 64\n"
        << "default rel\n"
        << "section .bss\n"
        << "buf: resb 30000\n"
        << "section .text\n"
        << "global main\n\n"
        << "main:\n"
        << "    lea rbx, [buf]\n";


    //simpler than I thought because I can directly modify the bytes
    //otherwise I would have to keep a value register and load store
    for (const auto& entry : IR){
        switch (entry.op)
        {
        case Op::Undefined:
            // maybe output a comment so I can see?
            out << "    ;Undefined Entry\n";
            break;
        case Op::AddCell:
            //is there a way to avoid emiting uneccesary labels?
            out << std::format("    add byte [rbx], {}\n", entry.arg);
            break;
        case Op::SubCell:
            out << std::format("    sub byte [rbx], {}\n", entry.arg);
            break;
        case Op::AddPtr:
            //increment pointer, remember to wrap the 30000
            out << std::format("    add rbx, {}\n", entry.arg)
                << "    lea rcx, [buf]\n"
                << "    mov rax, rbx\n"
                << "    sub rax, rcx\n"
                << "    cmp rax, 30000\n"
                << std::format("    jle .done{}\n", counter)
                << "    sub rbx, 30000\n"
                << std::format("    .done{}:\n", counter);
            ++counter;
            break;
        case Op::SubPtr:
            out << std::format("    sub rbx, {}\n", entry.arg)
                << "    lea rcx, [buf]\n"
                << "    cmp rbx, rcx\n"
                << std::format("    jge .done{}\n", counter)
                << "    add rbx, 30000\n"
                << std::format("    .done{}:\n", counter);
            ++counter;
            break;
        case Op::LoopStart:
            out << std::format("    loop_start{}:\n", entry.arg)
                << "    cmp byte [rbx], 0\n"
                << std::format("    je loop_end{}\n", entry.arg);
            break;
        case Op::LoopEnd:
            out << "    cmp byte [rbx], 0\n"
                << std::format("    jne loop_start{}\n", entry.arg)
                << std::format("    loop_end{}:\n", entry.arg);
            break;
        case Op::Input:
            //read syscall
            //if read returns 1, perfect
            //if read returns 0, store 0 in the cell
            //if read returns -1, store 0 in the cell
            out << "    mov rax, 0\n"
                << "    mov rdi, 0\n"
                << "    mov rsi, rbx\n"
                << "    mov rdx, 1\n"
                << "    syscall\n"
                << "    cmp rax, 1\n"
                << std::format("    je .done_input{}\n", counter)
                << "    mov byte [rbx], 0\n"
                << std::format("    .done_input{}:\n", counter); 
            ++counter;
            break;
        case Op::Output:
            out << "    mov rax, 1\n"
                << "    mov rdi, 1\n"
                << "    mov rsi, rbx\n"
                << "    mov rdx, 1\n"
                << "    syscall\n";
            break;
        }
    }

    //exit
    out << "    mov rax, 60\n"
        << "    xor rdi, rdi\n"
        << "    syscall\n"
        << "\nsection .note.GNU-stack noalloc noexec nowrite progbits\n";

    //dont forget to close
    out.close();

    // TEMPORARY
    system("nasm -f elf64 -g -F dwarf a.s -o a.o");
    system("g++ -g a.o -o a");
    system("rm -rf a.o");

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

    translate(IR);


    // TODO: build asm from IR

    return 0;
}
