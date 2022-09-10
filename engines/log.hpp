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
    enum logLevel {
        verbose,
        debug,
        info,
        warning,
        error
    };
    logLevel currentLogLevel = logLevel::debug;
    std::array<string, 5> logLevelNames = {"VERBOSE", "DEBUG", "INFO", "WARNING", "ERROR"};

    void printLogPrefix(logLevel level, const string &tag, std::ofstream &file) {
        auto timeStamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        file << std::put_time(std::localtime(&timeStamp), "%FT%T%z") << ":  " << logLevelNames[level] << "  " << tag << ":  ";
    }

    template<typename T>
    void printData(logLevel level, const string &tag, T data) {
        std::ofstream fout;
        fout.open(logFileName, std::ios::app);
        printLogPrefix(level, tag, fout);
        fout << data << std::endl;
        fout.close();
    }

    template<class T>
    void printData(logLevel level, const string &tag, std::initializer_list<T> &list) {
        std::ofstream fout;
        fout.open(logFileName, std::ios::app);
        printLogPrefix(level, tag, fout);
        for (auto &i : list) {
            fout << i << " ";
        }
        fout << std::endl;
        fout.close();
    }

    template<typename T>
    void printData(logLevel level, const string &tag, vector<T> &arg) {
        std::ofstream fout;
        fout.open(logFileName, std::ios::app);
        printLogPrefix(level, tag, fout);
        for (auto x: arg) {
            fout << x << ", ";
        }
        fout << std::endl;
        fout.close();
    }

    void setLogLevel(logLevel level) {
        currentLogLevel = level;
    }

    template<typename T>
    void v(const string &tag, T data) {
        if (currentLogLevel > logLevel::verbose) {
            return;
        }
        printData(logLevel::verbose, tag, data);
    }

    template<typename T>
    void v(const string &tag, std::initializer_list<T> &list) {
        if (currentLogLevel > logLevel::verbose) {
            return;
        }
        printData(logLevel::verbose, tag, list);
    }

    template<typename T>
    void v(const string &tag, vector<T> &arg) {
        if (currentLogLevel > logLevel::verbose) {
            return;
        }
        printData(logLevel::verbose, tag, arg);
    }

    template<typename T>
    void d(const string &tag, T data) {
        if (currentLogLevel > logLevel::debug) {
            return;
        }
        printData(logLevel::debug, tag, data);
    }

    template<typename T>
    void d(const string &tag, std::initializer_list<T> &list) {
        if (currentLogLevel > logLevel::debug) {
            return;
        }
        printData(logLevel::debug, tag, list);
    }

    template<typename T>
    void d(const string &tag, vector<T> &arg) {
        if (currentLogLevel > logLevel::debug) {
            return;
        }
        printData(logLevel::debug, tag, arg);
    }

    template<typename T>
    void i(const string &tag, T data) {
        if (currentLogLevel > logLevel::info) {
            return;
        }
        printData(logLevel::info, tag, data);
    }

    template<typename T>
    void i(const string &tag, std::initializer_list<T> &list) {
        if (currentLogLevel > logLevel::info) {
            return;
        }
        printData(logLevel::info, tag, list);
    }

    template<typename T>
    void i(const string &tag, vector<T> &arg) {
        if (currentLogLevel > logLevel::info) {
            return;
        }
        printData(logLevel::info, tag, arg);
    }

    template<typename T>
    void w(const string &tag, T data) {
        if (currentLogLevel > logLevel::warning) {
            return;
        }
        printData(logLevel::warning, tag, data);
    }

    template<typename T>
    void w(const string &tag, std::initializer_list<T> &list) {
        if (currentLogLevel > logLevel::warning) {
            return;
        }
        printData(logLevel::warning, tag, list);
    }

    template<typename T>
    void w(const string &tag, vector<T> &arg) {
        if (currentLogLevel > logLevel::warning) {
            return;
        }
        printData(logLevel::warning, tag, arg);
    }

    template<typename T>
    void e(const string &tag, T data) {
        if (currentLogLevel > logLevel::error) {
            return;
        }
        printData(logLevel::error, tag, data);
    }

    template<typename T>
    void e(const string &tag, std::initializer_list<T> &list) {
        if (currentLogLevel > logLevel::error) {
            return;
        }
        printData(logLevel::error, tag, list);
    }

    template<typename T>
    void e(const string &tag, vector<T> &arg) {
        if (currentLogLevel > logLevel::error) {
            return;
        }
        printData(logLevel::error, tag, arg);
    }
}
#endif