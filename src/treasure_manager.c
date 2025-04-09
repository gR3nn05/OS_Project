#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include "../include/treasure.h"

#define HUNTS_DIR "hunts"
#define TREASURE_FILE "treasures.dat"
#define LOG_FILE "log.txt"


void print_usage() {
    printf("Usage: treasure_manager --<operation> <args>\n");
}

void log_operation(const char *hunt_id, const char *message) {
    char log_path[512];
    snprintf(log_path, sizeof(log_path), "%s/%s/%s", HUNTS_DIR, hunt_id, LOG_FILE);

    int log_fd = open(log_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (log_fd == -1) {
        perror("Log open failed");
        return;
    }

    char log_entry[512];
    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strcspn(timestamp, "\n")] = '\0';  // Trim newline
    snprintf(log_entry, sizeof(log_entry), "[%s] %s\n", timestamp, message);
    

    write(log_fd, log_entry, strlen(log_entry));
    close(log_fd);

    // Create symlink if it doesn't exist
    // char symlink_name[256];
    // snprintf(symlink_name, sizeof(symlink_name), "log.txt%s", hunt_id);

    // if (access(symlink_name, F_OK) == -1) {
    //     char target[256];
    //     snprintf(target, sizeof(target), "%s/%s/%s", HUNTS_DIR, hunt_id, LOG_FILE);
    //     symlink(target, symlink_name);
    // }
}

void add_treasure(const char *hunt_id) {
    char dir_path[256];
    snprintf(dir_path, sizeof(dir_path), "%s/%s", HUNTS_DIR, hunt_id);

    // Create hunt directory if it doesn't exist
    mkdir(HUNTS_DIR, 0755); // Ensure base exists
    mkdir(dir_path, 0755);

    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, TREASURE_FILE);

    int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Failed to open treasure file");
        return;
    }

    Treasure t;
    printf("Enter Treasure ID: ");
    scanf("%d", &t.id);
    getchar(); // consume newline
    printf("Enter Username: ");
    fgets(t.username, MAX_USERNAME, stdin);
    t.username[strcspn(t.username, "\n")] = '\0'; // remove newline
    printf("Enter Latitude: ");
    scanf("%f", &t.latitude);
    printf("Enter Longitude: ");
    scanf("%f", &t.longitude);
    getchar(); // consume newline
    printf("Enter Clue: ");
    fgets(t.clue, MAX_CLUE_TEXT, stdin);
    t.clue[strcspn(t.clue, "\n")] = '\0';
    printf("Enter Value: ");
    scanf("%d", &t.value);

    // Write the struct as binary
    if (write(fd, &t, sizeof(Treasure)) != sizeof(Treasure)) {
        perror("Error writing treasure");
    } else {
        printf("Treasure was added successfully.\n");
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Added treasure ID %d to hunt %s", t.id, hunt_id);
        log_operation(hunt_id, log_msg);
    }

    close(fd);
}

void list_treasures(const char *hunt_id) {
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/%s/%s", HUNTS_DIR, hunt_id, TREASURE_FILE);

    // Get file info
    struct stat st;
    if (stat(file_path, &st) == -1) {
        perror("Could not access treasure file");
        return;
    }

    printf("Hunt: %s\n", hunt_id);
    printf("File size: %ld bytes\n", st.st_size);
    printf("Last modified: %s", ctime(&st.st_mtime));

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open treasure file");
        return;
    }

    Treasure t;
    printf("\nTreasures:\n");
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("ID: %d | User: %s | Location: (%.2f, %.2f) | Value: %d\n",
               t.id, t.username, t.latitude, t.longitude, t.value);
        printf("Clue: %s\n\n", t.clue);
    }

    close(fd);
}

void view_treasure(const char *hunt_id, int target_id) {
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/%s/%s", HUNTS_DIR, hunt_id, TREASURE_FILE);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open treasure file");
        return;
    }

    Treasure t;
    int found = 0;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.id == target_id) {
            printf("Treasure ID: %d\n", t.id);
            printf("User: %s\n", t.username);
            printf("Coordinates: (%.2f, %.2f)\n", t.latitude, t.longitude);
            printf("Clue: %s\n", t.clue);
            printf("Value: %d\n", t.value);
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("Treasure ID %d not found in hunt %s.\n", target_id, hunt_id);
    }

    close(fd);
}




int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "--add") == 0) {
        add_treasure(argv[2]);
    } else if (strcmp(argv[1], "--list") == 0) {
        list_treasures(argv[2]);
    } else if (strcmp(argv[1], "--view") == 0) {
        view_treasure(argv[2], atoi(argv[3]));
    } else if (strcmp(argv[1], "--remove_treasure") == 0) {
        // TODO: Remove treasure
    } else if (strcmp(argv[1], "--remove_hunt") == 0) {
        // TODO: Remove hunt
    } else {
        print_usage();
        return 1;
    }

    return 0;
}
