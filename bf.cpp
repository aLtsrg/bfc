#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

// read input line by line
// could collect line numbers or other information here
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


// TODO: add the mapping 
bool bracketsBalanced(std::string_view code)
{
    std::vector<int> stack{};

    for (const auto& c : code){
        if (c == '['){
            stack.push_back(c);
        } else if (c == ']'){
            if (stack.size() ==  0) return false;
            stack.pop_back();
        }
    }

    if (stack.size() > 0) return false;

    return true;
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

    std::cout  << bracketsBalanced(input) << '\n';

    return 0;
}
