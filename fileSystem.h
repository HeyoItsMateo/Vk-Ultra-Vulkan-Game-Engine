#ifndef hFileSystem
#define hFileSystem

#include <iostream>
#include <fstream>
#include <sstream>


//template<typename T>
//inline void writeFile(const char* fileName, T& data, std::ios::openmode openMode = std::ios::app) {// Console inputs are written to the file
//    std::ofstream _file(fileName);
//    if (_file.is_open()) {
//        std::string _write;
//        _file << data << '\n' << std::flush;
//        _file.close();
//    }
//    else std::cout << "Unable to open file";   
//}

void readFile(const char* fileName) {
    std::ifstream _file(fileName);
    if (_file.is_open()) {
        std::string _read;
        while (std::getline(_file, _read)) {
            std::cout << _read << '\n';
        }
        _file.close();
    }
    else std::cout << "Unable to open file";   
}

#endif