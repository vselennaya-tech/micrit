#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>

void cls() {
    printf("\033[H\033[2J");
    fflush(stdout);
}
int main() {
    cls();
    printf("\nMicrit v0.0 (C version)\n\n");
    // rootchk
    if (getuid() != 0) {
        fprintf(stderr, "[  ER  ] Must be ran as root!\n");
        exit(1);
    }
    // // rw
    printf("[ INFO ] Remounting rootfs read-write...\n");
    if (mount(NULL, "/", NULL, MS_REMOUNT, NULL) == 0) {
        printf("[  OK  ] Remounted rootfs successfully!\n");
    } else {
        perror("[  ER  ] Failed to remount rootfs");
    }
    // mkdirt //{proc, sys, dev, shm, devpts}
    mkdir("/proc", 0755);
    mkdir("/sys", 0755);
    mkdir("/dev", 0755);
    mkdir("/dev/pts", 0755);
    mkdir("/dev/shm", 0755);
    // //proc
    printf("[ INFO ] Mounting /proc...\n");
    if (mount("proc", "/proc", "proc", 0, NULL) == 0) {
        printf("[  OK  ] Mounted /proc!\n");
    } else {
        perror("[  ER  ] Failed to mount /proc");
    }
    // //sys
    printf("[ INFO ] Mounting /sys...\n");
    if (mount("sysfs", "/sys", "sysfs", 0, NULL) == 0) {
        printf("[  OK  ] Mounted /sys!\n");
    } else {
        perror("[  ER  ] Failed to mount /sys");
    }
    // //dev
    printf("[ INFO ] Mounting /dev...\n");
    if (mount("devtmpfs", "/dev", "devtmpfs", 0, NULL) == 0) {
        printf("[  OK  ] Mounted /dev!\n");
    } else {
        perror("[  ER  ] Failed to mount /dev");
    }
    // //dev/pts
    printf("[ INFO ] Mounting /dev/pts...\n");
    if (mount("devpts", "/dev/pts", "devpts", 0, "gid=5,mode=620") == 0) {
        printf("[  OK  ] Mounted /dev/pts!\n");
    } else {
        perror("[  ER  ] Failed to mount /dev/pts");
    }
    // //dev/shm
    printf("[ INFO ] Mounting /dev/shm...\n");
    if (mount("shm", "/dev/shm", "tmpfs", 0, NULL) == 0) {
        printf("[  OK  ] Mounted /dev/shm!\n");
    } else {
        perror("[  ER  ] Failed to mount /dev/shm");
    }
    // lo
    printf("[ INFO ] Setting loopback interface up...\n");
    if (system("ip link set lo up") == 0) {
        printf("[  OK  ] Set loopback interface up!\n");
    } else {
        fprintf(stderr, "[  ER  ] Failed to set up loopback interface!\n");
    }
    printf("[ INFO ] Finished booting!\n");
    // login
    while (1) {
        pid_t pid = fork();
        if (pid == 0) {
            setsid();
            int fd = open("/dev/tty1", O_RDWR);
            if (fd < 0) {
                perror("[  ER  ] Cannot open /dev/tty1");
                exit(1);
            }
            ioctl(fd, TIOCSCTTY, 1);
            dup2(fd, 0);
            dup2(fd, 1);
            dup2(fd, 2);
            if (fd > 2) close(fd);
            printf("[ INFO ] Running login...\n");
            cls();
            execl("/sbin/login", "login", "-p", NULL);
            perror("[  ER  ] Failed to start login");
            exit(1);
        } else if (pid > 0) {
            wait(NULL);
            cls();
        } else {
            perror("[  ER  ] Fork failed");
            sleep(2);
        }
    }
    return 0;
}
