//#define NDEBUG

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <cassert>
#include <format>
#include <unistd.h>
#include <sys/wait.h>


//TODO: find more optimizations
//      add LLVM IR backend
//      add tests

enum class Op {
    Undefined, 
    AddCell, 
    SubCell, 
    SetCell,
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

struct Options {
    std::string inputFile{};
    std::string outputFile      { "a.out" };
    std::string asmPath         { "a.s" };
    std::string objPath         { "a.o" };
    std::string irPath          { "a.ir" };
    bool emitIR  { false };
    bool emitASM { false };
    bool debug   { false };
};

// read input line by line
// could put other information here like line number etc.
// TODO: can this error? if so make it bool and return if error.
void readSourceFile(std::ifstream& bfFile, std::string& inputString)
{
    for (std::string line; std::getline(bfFile >> std::ws, line);)
        inputString += line + "\n";
}

void filter(std::string& inputString)
{
    std::string result{};
    for (const auto& c : inputString){
        if ( c == '+' || c == '-'|| c == '>' || c == '<' 
                || c == '[' || c == ']' || c == '.' || c == ',' )
            result += c;
    }
    inputString = result;
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

void buildIR(std::vector<IREntry>& IR, std::unordered_map<int, int>& idxToLpNbr, std::string_view code)
{
    int idx{};
    int cellCounter{};
    int pointCounter{};

    for ( const auto& c : code ){

        // =============================== run merging ===================================
        // maybe factor out into functions
        // treating it this way makes it so if the operations cancel eachother out, no IR is emitted
        if (c != '+' && c != '-' && cellCounter != 0){
            if (cellCounter > 0){
                IR.push_back({Op::AddCell, cellCounter});
            } else {
                IR.push_back({Op::SubCell, -cellCounter});
            }
            cellCounter = 0;
        }

        if (c != '<' && c != '>' && pointCounter != 0){
            if (pointCounter > 0){
                IR.push_back({Op::AddPtr, pointCounter});
            } else {
                IR.push_back({Op::SubPtr, -pointCounter});
            }
            pointCounter = 0;
        }
        // =============================== run merging ===================================

        switch (c)
        {
        case '+':
            //IR.push_back({Op::AddCell, 1});
            ++cellCounter;
            break;
        case '-':
            //IR.push_back({Op::SubCell, 1});
            --cellCounter;
            break;
        case '>':
            //IR.push_back({Op::AddPtr, 1});
            ++pointCounter;
            break;
        case '<':
            //IR.push_back({Op::SubPtr, 1});
            --pointCounter;
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

    //NOTE: cellCounter and pointCounter are not flushed, meaning trailing +, -, <, >, are not accounted for in the IR
    //      this is by design - these trailing characters do not have any observable impact on the program outputs.
    //      (however, if run merging is done for '.' or ',' this WOULD have an observable impact, in that case counters should be flushed)
}

//TODO: add more patterns?
void peephole(std::vector<IREntry>& IR)
{
    //if adding more in future should not increment after erase
    //could cause you to miss a pattern I guess?
    //i + 2, because we want to check IR.size() - 2 but causes wrapping issues when the IR is too small
    for (size_t i{}; i + 2 < IR.size(); ++i){
        if (IR[i].op == Op::LoopStart
                && ((IR[i+1].op == Op::AddCell && IR[i+1].arg == 1) || (IR[i+1].op == Op::SubCell && IR[i+1].arg == 1))
                && IR[i+2].op == Op::LoopEnd){
            IR[i] = {Op::SetCell, 0};
            IR.erase(IR.begin() + i + 1, IR.begin() + i + 3);
        }
    }
}

void emitIR(std::vector<IREntry>& IR, std::string irPath)
{
    int idx {};
    std::string opString {};
    std::ofstream out(irPath);
    for(const auto& entry : IR){
        
        if (entry.op == Op::Undefined) opString = "Undefined";
        if (entry.op == Op::AddCell) opString = "AddCell";
        if (entry.op == Op::SubCell) opString = "SubCell";
        if (entry.op == Op::SetCell) opString = "Set";
        if (entry.op == Op::AddPtr) opString = "AddPtr";
        if (entry.op == Op::SubPtr) opString = "SubPtr";
        if (entry.op == Op::LoopStart) opString = "LoopStart";
        if (entry.op == Op::LoopEnd) opString = "LoopEnd";
        if (entry.op == Op::Output) opString = "Output";
        if (entry.op == Op::Input) opString = "Input";
        
        out << idx << ": " << opString << ',' << entry.arg << '\n';
        ++idx;
    }
}

// helper for pretty printing the asm
std::string indent(size_t level)
{
    return std::string(level, '\t');
}

void translate(const std::vector<IREntry>& IR, std::ostream& out)
{
    //counter for label generation
    int counter{};

    //indentation level for pretty printing
    size_t level{ 1 };

    // format example:
    //     out << std::format("Hello {}\n", 69);
    
    // function should:
    // go through IR generating asm per IREntry
    
    // header
    out << "bits 64\n"
        << "default rel\n"
        << "section .bss\n"
        << "buf: resb 30000\n"
        << "section .text\n"
        << "global main\n\n"
        << "main:\n"
        << indent(level) << "lea rbx, [buf]\n";


    //simpler than I thought because I can directly modify the bytes
    //otherwise I would have to keep a value register and load store
    for (const auto& entry : IR){
        switch (entry.op)
        {
        case Op::Undefined:
            // maybe output a comment so I can see?
            out << indent(level * 2) << ";Undefined Entry\n";
            break;
        case Op::AddCell:
            out << indent(level) << std::format("add byte [rbx], {}\n", entry.arg);
            break;
        case Op::SubCell:
            out << indent(level) << std::format("sub byte [rbx], {}\n", entry.arg);
            break;
        case Op::SetCell:
            //will this ever not be 0?
            out << indent(level) << std::format("mov byte [rbx], {}\n", entry.arg);
            break;
        case Op::AddPtr:
            //increment pointer, remember to wrap the 30000
            out << indent(level) << std::format("add rbx, {}\n", entry.arg)
                << indent(level) << "lea rcx, [buf]\n"
                << indent(level) << "mov rax, rbx\n"
                << indent(level) << "sub rax, rcx\n"
                << indent(level) << "cmp rax, 30000\n"
                << indent(level) << std::format("jle .done{}\n", counter)
                << indent(level) << "sub rbx, 30000\n"
                << indent(level) << std::format(".done{}:\n", counter);
            ++counter;
            break;
        case Op::SubPtr:
            out << indent(level) << std::format("sub rbx, {}\n", entry.arg)
                << indent(level) << "lea rcx, [buf]\n"
                << indent(level) << "cmp rbx, rcx\n"
                << indent(level) << std::format("jge .done{}\n", counter)
                << indent(level) << "add rbx, 30000\n"
                << indent(level) << std::format(".done{}:\n", counter);
            ++counter;
            break;
        case Op::LoopStart:
            out << indent(level) << std::format("loop_start{}:\n", entry.arg);
            ++level;
            out << indent(level) << "cmp byte [rbx], 0\n"
                << indent(level) << std::format("je loop_end{}\n", entry.arg);
            break;
        case Op::LoopEnd:
            out << indent(level) << "cmp byte [rbx], 0\n"
                << indent(level) << std::format("jne loop_start{}\n", entry.arg);
            --level;
            out << indent(level) << std::format("loop_end{}:\n", entry.arg);
            break;
        case Op::Input:
            //read syscall
            //if read returns 1, perfect
            //if read returns 0, store 0 in the cell
            //if read returns -1, store 0 in the cell
            out << indent(level) << "mov rax, 0\n"
                << indent(level) << "mov rdi, 0\n"
                << indent(level) << "mov rsi, rbx\n"
                << indent(level) << "mov rdx, 1\n"
                << indent(level) << "syscall\n"
                << indent(level) << "cmp rax, 1\n"
                << indent(level) << std::format("je .done_input{}\n", counter)
                << indent(level) << "mov byte [rbx], 0\n"
                << indent(level) << std::format(".done_input{}:\n", counter); 
            ++counter;
            break;
        case Op::Output:
            out << indent(level) << "mov rax, 1\n"
                << indent(level) << "mov rdi, 1\n"
                << indent(level) << "mov rsi, rbx\n"
                << indent(level) << "mov rdx, 1\n"
                << indent(level) << "syscall\n";
            break;
        default:
            std::cerr << "\033[1;31mERROR: \033[0m" << "UNREACHABLE CASE\n";
            break;
        }
    }

    //exit
    out << indent(level) << "mov rax, 60\n"
        << indent(level) << "xor rdi, rdi\n"
        << indent(level) << "syscall\n"
        << indent(level) << "\nsection .note.GNU-stack noalloc noexec nowrite progbits\n";
}

void emitASM(std::string_view asmString, const std::string& asmPath)
{
    //no need to close manually closes when function returns, only when needed before that
    std::ofstream out(asmPath);
    out << asmString;
}

//TODO: add error checking
void run(const std::vector<const char*>& args)
{
    pid_t pid = fork();

    if(pid ==  0){
        std::vector<char*> argv{};
        argv.reserve(args.size() + 1);

        for (auto s : args){
            argv.push_back(const_cast<char*>(s));
        }
        argv.push_back(nullptr);

        execvp(argv[0], argv.data());

        //make error more useful
        perror("execvp failed");
        _exit(1);
    }

    waitpid(pid, nullptr, 0);
}

void assembleASM(const Options& options)
{
    if(options.debug){
        run({"nasm",
             "-f", "elf64",
             "-g", "-F", "dwarf",
             options.asmPath.c_str(), 
             "-o",
             options.objPath.c_str()});
    } else {
        run({"nasm",
             "-f", "elf64",
             options.asmPath.c_str(), 
             "-o",
             options.objPath.c_str()});
    }
}

void link(const Options& options)
{
    if(options.debug){
        run({"g++",
             "-ggdb",
             options.objPath.c_str(), 
             "-o",
             options.outputFile.c_str()});
    } else {
        run({"g++",
             options.objPath.c_str(), 
             "-o",
             options.outputFile.c_str()});
    }
}

void cleanUp(const Options& options)
{
    if(!options.emitIR){
        run({"rm",
             "-rf",
             options.irPath.c_str() });
    }
    if(!options.emitASM && !options.debug){
        run({"rm",
             "-rf",
             options.asmPath.c_str() });
    }
    run({"rm", "-rf", options.objPath.c_str() });
}

int main(int argc, char *argv[])
{

    Options options{};
    std::vector<std::string> positional{};

    for(int i{1}; i < argc; ++i){
        std::string_view arg {argv[i]};

        if(arg.starts_with('-')){
            if(arg == "-h" || arg == "--help"){
                std::cout << "  bfc - brainfuck compiler\n"
                          << "  Usage:\n"
                          << "    bfc [options] <input>\n\n"
                          << "  Options:\n"
                          << "    -h, --help                    shows this menu\n"
                          << "    -o, --output FILENAME         set the name for the executable\n"
                          << "    -i, --emit-ir                 emit the IR\n"
                          << "    -a, --emit-asm                emit the NASM\n"
                          << "    -g                            add debug information to build\n\n"
                          << "  Examples:\n"
                          << "    bfc hello.bf\n"
                          << "    bfc hello.bf -o hello\n";
                exit(0);
            }
            else if (arg == "-o" || arg == "--output"){
                //make sure we have a next argument and it is not a flag
                if(++i < argc && !(argv[i][0] == '-')){
                    std::string outName {argv[i]};
                    size_t pos = outName.rfind('.');
                    if(pos == std::string_view::npos || outName.substr(pos) == ".out"){
                        options.outputFile = outName;
                        options.asmPath = outName + ".s";
                        options.objPath = outName + ".o";
                        options.irPath  = outName + ".ir";
                    } else {
                        std::cerr << "\033[1;31mERROR:\033[0m Incorrect output file type provided\033[0m" << std::endl;
                        return 1;
                    }
                } else {
                    std::cerr << "\033[1;31mERROR:\033[0m No output file provided\033[0m" << std::endl;
                    return 1;
                }
            }
            else if (arg == "-i" || arg == "--emit-ir"){
                options.emitIR = true;
            }
            else if (arg == "-a" || arg == "--emit-asm"){
                options.emitASM = true;
            } 
            else if (arg == "-g"){
                options.debug = true;
            }
            else {
                std::cerr << "\033[1;31mERROR:\033[0m unknown flag \033[1;33m`" << arg << "`\033[0m" << std::endl;
                return 1;
            }
        }
        else {
            //positional[0] should be the input file
            positional.push_back(argv[i]);
        }
        
    }

    //add more checks if more positional arguments are added
    if (positional.size() > 0 && positional[0].ends_with(".bf")){
        options.inputFile = positional[0];
    } else {
        std::cerr << "\033[1;31mERROR:\033[0m Source file not provided, try: \033[1;33m`bfc --help`\033[0m"  << std::endl;
        return 1;
    }

    //idk about this
    std::string input{};
    std::ifstream bfFile { options.inputFile };
    if (!bfFile){
        std::cerr << "\033[1;31mERROR: \033[0m" << "File called \033[1;33m`"
            << options.inputFile << "`\033[0m was not found\n";
        return 1;
    }

    readSourceFile(bfFile, input); 
    //std::cout << "RAW INPUT: \n" << input << '\n';

    filter(input); 
    //std::cout << "FILTERED: \n" << input << '\n';
    
    std::unordered_map<int, int> indexToLoopNumber{};
    if (!validateAndMapBrackets(input, indexToLoopNumber)){
        std::cerr << "\033[1;31mSYNTAX ERROR: \033[0m" << "brackets not balanced\n";
        return 1;
    }

    std::vector<IREntry> IR{};
    buildIR(IR, indexToLoopNumber, input);

    peephole(IR);
    
    emitIR(IR, options.irPath);

    //TODO: look into string streams more
    std::ostringstream ss;
    translate(IR, ss);
    std::string asmString = ss.str();

    //for seperation
    //if I add LLVM IR then this could also emit that
    //would need to pass flag then too
    emitASM(asmString, options.asmPath);
    assembleASM(options);

    link(options);

    cleanUp(options);

    return 0;
}
