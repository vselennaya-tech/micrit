#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/reboot.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>

int get_config_value(const char *filename, const char *key, char *buffer, size_t max_len) {
    FILE *file = fopen(filename, "r");
    if (!file) return 0; // if !file
    char line[256];
    int found = 0;
    while (fgets(line, sizeof(line), file)) {
        char *ptr = line;
        while (isspace(*ptr)) ptr++;
        if (*ptr == '#' || *ptr == '\0') continue;
// get config values
        if (strncmp(ptr, key, strlen(key)) == 0) {
            char *equal_sign = strchr(ptr, '=');
            if (equal_sign) {
                char *val = equal_sign + 1;
                while (isspace(*val)) val++;
                char *end = val + strlen(val) - 1;
                while (end > val && isspace(*end)) {
                    *end = '\0';
                    end--;
                }
                strncpy(buffer, val, max_len - 1);
                buffer[max_len - 1] = '\0';
                found = 1;
                break;
            }
        }
    }
    fclose(file);
    return found;
}
void cls() {
    printf("\033[H\033[2J");
    fflush(stdout);
}
pid_t spawn_tty(const char *tty_path) {
    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        int fd = open(tty_path, O_RDWR);
        if (fd < 0) {
            fprintf(stderr, "[  ER  ] Cannot open %s\n", tty_path);
            exit(1);
        }
        ioctl(fd, TIOCSCTTY, 1);
        dup2(fd, 0);
        dup2(fd, 1);
        dup2(fd, 2);
        if (fd > 2) close(fd);
        cls();
        setenv("TERM", "linux", 1);
        setenv("PATH", "/usr/local/bin:/usr/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin", 1);
        execl("/sbin/login", "login", "-p", NULL);
        perror("[  ER  ] Failed to start login");
        exit(1);
    } else if (pid < 0) {
        perror("[  ER  ] Fork failed");
    }
    return pid; 
}
volatile sig_atomic_t power_action = 0;

void power_signals(int sig) {
    if (sig == SIGINT) power_action = 1;
    if (sig == SIGUSR1) power_action = 2;
}
int main() {
    cls();
    printf("\nMicrit v0.01 (C version)\n\n");
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
// mkdir //proc, //sys, //dev, //dev/shm, //dev/pts
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
// loopback
    printf("[ INFO ] Setting loopback interface up...\n");
    if (system("ip link set lo up") == 0) {
        printf("[  OK  ] Set loopback interface up!\n");
    } else {
        fprintf(stderr, "[  ER  ] Failed to set up loopback interface!\n");
    }
// hostname (default: localhost)
    char hostname[64] = "localhost"; // default
    if (get_config_value("/etc/micrit.conf", "HOSTNAME", hostname, sizeof(hostname))) {
        printf("[ INFO ] Setting hostname from config: %s\n", hostname);
    } else {
        printf("[ WARN ] HOSTNAME not found in config, using default\n");
    }
    sethostname(hostname, strlen(hostname));
// kernel modules
    printf("[ INFO ] Loading kernel modules...\n");
    if (system("modprobe -ab --all") == 0) {
        printf("[  OK  ] Loaded kernel modules!\n");
    } else {
        fprintf(stderr, "[  ER  ] Failed to load kernel modules!");
    }
// udev (default: sytemd-udevd)
    char udev_path[128] = "/usr/lib/systemd/systemd-udevd";
    get_config_value("/etc/micrit.conf", "UDEV_DAEMON", udev_path, sizeof(udev_path));
    printf("[ INFO ] Starting device manager: %s\n", udev_path);
    char udev_cmd[256];
    snprintf(udev_cmd, sizeof(udev_cmd), "%s --daemon", udev_path);
    if (system(udev_cmd) == 0) {
        system("udevadm trigger --action=add --type=subsystems");
        system("udevadm trigger --action=add --type=devices");
        system("udevadm settle --timeout=10");
    } else {
        fprintf(stderr, "[  ER  ] Failed to start udev daemon!\n");
    }
// inet devices
    char interfaces[256] = "";
    if (get_config_value("/etc/micrit.conf", "INTERFACES", interfaces, sizeof(interfaces))) {
        char *iface = strtok(interfaces, " ");
        while (iface != NULL) {
            printf("[ INFO ] Bringing up interface: %s\n", iface);
            char ip_cmd[128];
            snprintf(ip_cmd, sizeof(ip_cmd), "ip link set %s up", iface);
            system(ip_cmd);
            iface = strtok(NULL, " ");
        }
    }
// power signals
    signal(SIGINT, power_signals);
    signal(SIGUSR1, power_signals);
    printf("[ INFO ] Finished booting!\n");
    // login
    const char *tty_devices[] = {
        "/dev/tty1", "/dev/tty2", "/dev/tty3",
        "/dev/tty4", "/dev/tty5", "/dev/tty6"
    };
    #define NUM_TTYS (sizeof(tty_devices) / sizeof(tty_devices[0]))
    pid_t tty_pids[NUM_TTYS] = {0};
    for (int i = 0; i < NUM_TTYS; i++) {
        tty_pids[i] = spawn_tty(tty_devices[i]);
    }
    while (1) {
        int status;
        pid_t died_pid = waitpid(-1, &status, WNOHANG);
        if (died_pid > 0) {
            for (int i = 0; i < NUM_TTYS; i++) {
                if (tty_pids[i] == died_pid) {
                    tty_pids[i] = spawn_tty(tty_devices[i]);
                    break;
                }
            }
        }
        if (power_action > 0) {
            cls();
            kill(-1, SIGTERM); sleep(2);
            kill(-1, SIGKILL); sleep(1);
            sync();
            NULL, "/", NULL, MS_REMOUNT | MS_RDONLY, NULL;
            if (power_action == 1) reboot(RB_AUTOBOOT);
            if (power_action == 2) reboot(RB_POWER_OFF);
        }
        sleep(1);
    }
    return 0;
}
