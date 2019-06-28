#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <string>

using namespace std;

enum log_level {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

class Logger {
    private:
        static Logger* instance;
        log_level level;

    public:
        static Logger* get();
        Logger();
        void set_level(log_level level);
        void log(log_level level, string msg);
        void error(string msg, string file, size_t line);
};

#endif
