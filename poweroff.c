#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int main() {
    if (getuid() == 0) {
        kill(1, SIGUSR1);
        return 0;
    }
    else {
        fprintf(stderr, "[  ER  ] Must be ran as root!\n");
        return 1;
    }
}
