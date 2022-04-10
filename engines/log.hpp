#include <chrono>
#include <fstream>
#include <vector>

using std::string;
using std::vector;

void log(string str) {
    //const auto now = std::chrono::system_clock::now();
    std::ofstream fout;
    fout.open("logs.txt", std::ios::app);
    fout << str << std::endl;
    fout.close();
}

void log(int num) {
    log(std::to_string(num));
}

template<typename T>
void log(vector<T> &arg) {
    std::string str = "";

    for (auto x: arg) {
        str += std::to_string((int)x) + ",";
    }
    log(str);
}