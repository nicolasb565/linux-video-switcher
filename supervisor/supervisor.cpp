#include <csignal>

#include <atomic>
#include <iostream>

std::atomic_bool stop_process = false;

void sighandler(int signum) {
    stop_process = true;
}

void spawnChildProcesses() {
    //spawn missing child processes
}

void stopChildProcesses() {
    //stop all child processes
}

void handleStuckProcesses() {
    //we handle case where we have an unresponsive process by killing it
}

int main() {
    while(!stop_process) {
        spawnChildProcesses();
        handleStuckProcesses();
        usleep(100 * 1000);
    }
    stopChildProcesses();
}
