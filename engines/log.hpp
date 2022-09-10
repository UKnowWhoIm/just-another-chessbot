#ifndef CHESS_LOG
#define CHESS_LOG 1
#include <iomanip>
#include <chrono>
#include <fstream>
#include <vector>

using std::string;
using std::vector;

namespace logging {
    const string logFileName = "/app/logs.txt";

    template<typename T>
    void printData(T data) {
        auto timeStamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::ofstream fout;
        fout.open(logFileName, std::ios::app);
        fout << std::put_time(std::localtime(&timeStamp), "%FT%T%z") << ": " << data << std::endl;
        fout.close();
    }

    template<class T>
    void printData(std::initializer_list<T> list) {
        auto timeStamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::ofstream fout;
        fout.open(logFileName, std::ios::app);
        fout << std::put_time(std::localtime(&timeStamp), "%FT%T%z") << ": ";
        for (auto &i : list) {
            fout << i << " ";
        }
        fout << std::endl;
        fout.close();
    }

    template<typename T>
    void printData(vector<T> &arg) {
        std::string str = "";

        for (auto x: arg) {
            str += std::to_string((int)x) + ",";
        }
        printData(str);
    }
}
#endif