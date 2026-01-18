#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>

enum LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3
};

class Logger {
private:
    static LogLevel current_level;

public:
    static void set_level(LogLevel level);
    
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);
    
private:
    static void log(LogLevel level, const std::string& message);
    static std::string get_timestamp();
    static std::string level_to_string(LogLevel level);
};

#endif // LOGGER_H
