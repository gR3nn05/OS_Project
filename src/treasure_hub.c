#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>

typedef struct {
    int id;
    char username[128];
    float latitude;
    float longitude;
    char clue[128];
    int value;
} Treasure;

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
                usleep(100000); // Wait for monitor to process
            } else {
                printf("Error: Monitor is not running.\n");
            }
        }

        else if (strcmp(command, "list_treasures") == 0) {
            if (monitor_pid > 0) {
                kill(monitor_pid, SIGUSR2);
                usleep(100000); // Wait for monitor to process
            } else {
                printf("Error: Monitor is not running.\n");
            }
        }

        else if (strcmp(command, "view_treasure") == 0) {
            if (monitor_pid > 0) {
                kill(monitor_pid, SIGUSR2);
                usleep(100000); // Wait for monitor to process
            } else {
                printf("Error: Monitor is not running.\n");
            }
        }

        else if (strcmp(command, "stop_monitor") == 0) {
            if (monitor_pid > 0) {
                kill(monitor_pid, SIGTERM);
                printf("Stopping monitor...\n");
                waitpid(monitor_pid, NULL, 0); // Wait for monitor to exit
                monitor_pid = -1; // Reset monitor PID
            } else {
                printf("Error: Monitor is not running.\n");
            }
        }

        else if (strcmp(command, "exit") == 0) {
            if (monitor_pid > 0) {
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

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sa.sa_handler = handle_list_hunts;
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_handler = handle_list_treasures;
    sigaction(SIGUSR2, &sa, NULL);

    sa.sa_handler = handle_sigterm;
    sigaction(SIGTERM, &sa, NULL);

    while (1) {
        pause();
    }
}

void handle_list_hunts(int sig) {
    (void)sig;
    printf("[Monitor] Listing hunts...\n");

    DIR *dir = opendir("hunts");
    if (!dir) {
        perror("[Monitor] Failed to open hunts directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strncmp(entry->d_name, "hunt", 4) == 0) {
            printf("[Monitor] Found hunt: %s\n", entry->d_name);
        }
    }

    closedir(dir);
}

void handle_list_treasures(int sig) {
    (void)sig;
    printf("[Monitor] Listing treasures...\n");

    DIR *dir = opendir("hunts");
    if (!dir) {
        perror("[Monitor] Failed to open hunts directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strncmp(entry->d_name, "hunt", 4) == 0) {
            printf("[Monitor] Hunt: %s\n", entry->d_name);

            char file_path[512];
            snprintf(file_path, sizeof(file_path), "hunts/%s/treasures.dat", entry->d_name);

            int fd = open(file_path, O_RDONLY);
            if (fd == -1) {
                perror("[Monitor] Error opening treasure file");
                continue;
            }

            Treasure t;
            printf("\n  Treasures:\n");
            while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
                printf("    ID: %d | User: %s | Location: (%.2f, %.2f) | Value: %d\n",
                       t.id, t.username, t.latitude, t.longitude, t.value);
                printf("    Clue: %s\n\n", t.clue);
            }

            close(fd);
        }
    }

    closedir(dir);
}

void handle_view_treasure(int sig) {
    (void)sig;
    printf("[Monitor] Viewing specific treasure...\n");

    char hunt_id[128];
    int treasure_id;
    printf("Enter hunt ID: ");
    scanf("%s", hunt_id);
    printf("Enter treasure ID: ");
    scanf("%d", &treasure_id);

    char file_path[512];
    snprintf(file_path, sizeof(file_path), "hunts/%s/treasures.dat", hunt_id);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("[Monitor] Error opening treasure file");
        return;
    }

    Treasure t;
    int found = 0;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.id == treasure_id) {
            printf("[Monitor] Treasure Details:\n");
            printf("  ID: %d\n", t.id);
            printf("  User: %s\n", t.username);
            printf("  Coordinates: (%.2f, %.2f)\n", t.latitude, t.longitude);
            printf("  Clue: %s\n", t.clue);
            printf("  Value: %d\n", t.value);
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("[Monitor] Treasure ID %d not found in hunt %s.\n", treasure_id, hunt_id);
    }

    close(fd);
}

void handle_sigterm(int sig) {
    (void)sig;
    printf("[Monitor] Terminating monitor...\n");
    usleep(500000); // Simulate delayed termination
    exit(0);
}

void sigchld_handler(int sig) {
    (void)sig;
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