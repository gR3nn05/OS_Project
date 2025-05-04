#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>

pid_t monitor_pid = -1;
volatile sig_atomic_t monitor_exiting = 0;


void run_monitor();
void handle_list_hunts(int sig);
void handle_list_treasures(int sig);
void handle_view_treasure(int sig);
void handle_sigterm(int sig);
void sigchld_handler(int sig);

void run_hub() {
    char command[128];

    while (1) {
        printf("hub> ");
        fflush(stdout);

        if (!fgets(command, sizeof(command), stdin)) break;
        command[strcspn(command, "\n")] = 0;

        if (strcmp(command, "start_monitor") == 0) {
            if (monitor_pid > 0) {
                printf("Monitor already running (PID %d)\n", monitor_pid);
                continue;
            }

            monitor_pid = fork();
            if (monitor_pid == 0) {
                // Child becomes the monitor
                run_monitor();
                exit(0);
            } else if (monitor_pid > 0) {
                printf("Started monitor with PID %d\n", monitor_pid);
            } else {
                perror("fork");
            }
        }

        else if (strcmp(command, "list_hunts") == 0) {
            if (monitor_pid > 0) {
                kill(monitor_pid, SIGUSR1);
            } else {
                printf("Error: Monitor is not running.\n");
            }
        }

        else if (strcmp(command, "list_treasures") == 0) {
            if (monitor_pid > 0) {
                kill(monitor_pid, SIGUSR2);
            } else {
                printf("Error: Monitor is not running.\n");
            }
        }

        else if (strcmp(command, "view_treasure") == 0) {
            if (monitor_pid > 0) {
                kill(monitor_pid, SIGUSR2); // Placeholder for specific signal
            } else {
                printf("Error: Monitor is not running.\n");
            }
        }

        else if (strcmp(command, "stop_monitor") == 0) {
            if (monitor_pid > 0) {
                kill(monitor_pid, SIGTERM);
                printf("Stopping monitor...\n");
            } else {
                printf("Error: Monitor is not running.\n");
            }
        }

        else if (strcmp(command, "exit") == 0) {
            if (monitor_pid > 0 && !monitor_exiting) {
                printf("Error: Monitor is still running. Use stop_monitor first.\n");
            } else {
                break;
            }
        }

        else {
            printf("Unknown command: %s\n", command);
        }
    }
}

void run_monitor() {
    printf("[Monitor] Monitor process started. PID = %d\n", getpid());

    // Setup signal handlers
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sa.sa_handler = handle_list_hunts;
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_handler = handle_list_treasures;
    sigaction(SIGUSR2, &sa, NULL);

    sa.sa_handler = handle_sigterm;
    sigaction(SIGTERM, &sa, NULL);

    // Wait for signals
    while (1) {
        pause(); // wait for signal
    }
}

void handle_list_hunts(int sig) {
    (void)sig; // Suppress unused parameter warning
    printf("[Monitor] Received list_hunts command (SIGUSR1)\n");

    // Open the current directory to list hunts
    DIR *dir = opendir(".");
    if (!dir) {
        perror("[Monitor] Failed to open directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strncmp(entry->d_name, "Hunt", 4) == 0) {
            char hunt_path[256];
            snprintf(hunt_path, sizeof(hunt_path), "%s/treasures.dat", entry->d_name);

            int fd = open(hunt_path, O_RDONLY);
            if (fd < 0) {
                perror("[Monitor] Failed to open treasure file");
                continue;
            }

            int treasure_count = 0;
            lseek(fd, 0, SEEK_SET);
            struct stat st;
            fstat(fd, &st);

            while (read(fd, NULL, sizeof(int)) > 0) {
                treasure_count++;
            }

            close(fd);
            printf("[Monitor] Hunt: %s, Total Treasures: %d, File Size: %ld bytes\n",
                   entry->d_name, treasure_count, st.st_size);
        }
    }

    closedir(dir);
}

void handle_list_treasures(int sig) {
    (void)sig; // Suppress unused parameter warning
    printf("[Monitor] Received list_treasures command (SIGUSR2)\n");
    // TODO: Implement logic to list treasures in a hunt
}

void handle_view_treasure(int sig) {
    (void)sig; // Suppress unused parameter warning
    printf("[Monitor] Received view_treasure command\n");
    // TODO: Implement logic to view a specific treasure
}

void handle_sigterm(int sig) {
    (void)sig; // Suppress unused parameter warning
    printf("[Monitor] Terminating monitor after delay...\n");
    usleep(500000); // Simulate delay
    exit(0);
}

void sigchld_handler(int sig) {
    (void)sig; // Suppress unused parameter warning
    int status;
    waitpid(monitor_pid, &status, 0);
    monitor_exiting = 1;
    printf("[Hub] Monitor exited with status %d\n", status);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGCHLD, &sa, NULL);

    run_hub();
    return 0;
}