#include "config.h"
#include "logger.h"
#include "execgen.h"
#include "seedgen.h"

int main(int argc, char* argv[]) {

    Logger::get()->set_level(DEBUG);

    if(!Config::get()->parse_cmd(argc, argv)) {
        Config::get()->print_usage();
        return 0;
    }

    if(!Config::get()->parse_conf()) {
        return 0;
    }

    if(Config::get()->is_exec()) {
        ExecGen::get()->generate();
    }
    else {
        SeedGen::get()->generate();
    }

    return 0;
}
