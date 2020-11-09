#include <gflags/gflags.h>

#include "naruto.h"

int main(int argc, char** argv){
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    naruto::server->run();
    delete naruto::server;
    return 0;
}
