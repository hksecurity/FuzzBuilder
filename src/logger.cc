#include "logger.h"

#include <assert.h>
#include <iostream>

using namespace std;

Logger* Logger::instance = nullptr;

Logger* Logger::get() {
    if (instance == nullptr) {
        instance = new Logger();
    }
    return instance;
}

Logger::Logger() {
    this->level = INFO;
}

void Logger::set_level(log_level level) {
    this->level = level;
}

void Logger::log(log_level level, string msg) {
    if (level < this->level) {
        return;
    }

    string prefix;
    switch(level) {
        case DEBUG:
            prefix = "[D] ";
            break;
        case INFO:
            prefix = "[I] ";
            break;
        case WARN:
            prefix = "[W] ";
            break;
        case ERROR:
            prefix = "[E] ";
            break;
        case FATAL:
            prefix = "[F] ";
            break;
        default:
            this->error("Unexpected state", __FILE__, __LINE__);
            exit(1);
    }
    cout << prefix << msg << "\n";
}

void Logger::error(string msg, string file, size_t line) {
    this->log(FATAL, msg + "(" + file + ":" + to_string(line) +")");
}
