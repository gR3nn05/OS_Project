#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include "monitor.h"

// Forward declaration 
void view_specific_treasure(const char *hunt_id, int treasure_id);

void handle_sigterm(int sig) {
    (void)sig;
    printf("[Monitor] Terminating...\n");
    fflush(stdout);
    // Adding a small delay to simulate delayed termination
    usleep(500000); // 0.5 seconds
    exit(0);
}

void run_monitor() {
    // Setup signal handler
    struct sigaction sa;
    sa.sa_handler = handle_sigterm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    
    printf("[Monitor] Monitor process started. PID = %d\n", getpid());
    fflush(stdout);

    char command[128];
    while (fgets(command, sizeof(command), stdin)) {
        // Remove newline if present
        size_t len = strlen(command);
        if (len > 0 && command[len-1] == '\n') {
            command[len-1] = '\0';
            len--;
        }
        
        if (len == 0) continue; // Skip empty commands
        
        printf("[Monitor] Received command: %s\n", command);
        fflush(stdout);

        if (strcmp(command, "list_hunts") == 0) {
            handle_list_hunts(0);
        } else if (strcmp(command, "list_treasures") == 0) {
            list_all_treasures();
        } else if (strncmp(command, "view_treasure", 13) == 0) {
            // Extract hunt_id and treasure_id if they are provided
            char hunt_id[32] = "";
            int treasure_id = -1;
            
            if (sscanf(command + 13, " %31s %d", hunt_id, &treasure_id) == 2) {
                view_specific_treasure(hunt_id, treasure_id);
            } else {
                // Show usage instead of interactive prompts
                printf("[Monitor] Usage: view_treasure <hunt_id> <treasure_id>\n");
                printf("[Monitor] Available hunts:\n");
                
                DIR *dir = opendir("hunts");
                if (dir) {
                    struct dirent *entry;
                    while ((entry = readdir(dir)) != NULL) {
                        if (entry->d_type == DT_DIR && strncmp(entry->d_name, "hunt", 4) == 0) {
                            printf("  - %s\n", entry->d_name);
                        }
                    }
                    closedir(dir);
                }
                fflush(stdout);
            }
        } else if (strcmp(command, "help") == 0) {
            printf("[Monitor] Available commands:\n");
            printf("  list_hunts       - List all treasure hunts\n");
            printf("  list_treasures   - List all treasures in all hunts\n");
            printf("  view_treasure <hunt_id> <treasure_id> - View details of a specific treasure\n");
            fflush(stdout);
        } else {
            printf("[Monitor] Unknown command: %s\n", command);
            printf("[Monitor] Type 'help' for a list of available commands\n");
            fflush(stdout);
        }
    }
    
    printf("[Monitor] End of input, terminating.\n");
    fflush(stdout);
}

void handle_list_hunts(int sig) {
    (void)sig;
    printf("[Monitor] Listing hunts...\n");

    DIR *dir = opendir("hunts");
    if (!dir) {
        perror("[Monitor] Failed to open hunts directory");
        fflush(stdout);
        return;
    }

    struct dirent *entry;
    int hunt_count = 0;
    
    printf("\n=== Available Treasure Hunts ===\n");
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strncmp(entry->d_name, "hunt", 4) == 0) {
            printf("[Hunt] %s\n", entry->d_name);
            
            // Count treasures in this hunt
            char file_path[512]; // Increase buffer size
            // Make sure the path won't be truncated
            if (snprintf(file_path, sizeof(file_path), "hunts/%s/treasures.dat", entry->d_name) >= (int)sizeof(file_path)) {
                printf("  Path too long, skipping treasure count\n");
                continue;
            }
            
            int fd = open(file_path, O_RDONLY);
            if (fd != -1) {
                // Get file size
                struct stat st;
                fstat(fd, &st);
                
                // Count treasures (assuming each treasure is sizeof(Treasure) bytes)
                int treasure_count = st.st_size / sizeof(Treasure);
                printf("  Total treasures: %d\n", treasure_count);
                
                // Get last modification time
                char time_buf[64];
                strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));
                printf("  Last modified: %s\n", time_buf);
                printf("  File size: %ld bytes\n", st.st_size);
                
                close(fd);
            } else {
                printf("  No treasure data available\n");
            }
            
            hunt_count++;
            printf("\n");
        }
    }
    
    if (hunt_count == 0) {
        printf("No treasure hunts found.\n");
    }
    
    printf("\nTotal hunts: %d\n", hunt_count);
    closedir(dir);
    fflush(stdout);
}

// Updated function to list treasures using the exact same approach as treasure_manager
void list_all_treasures() {
    printf("[Monitor] Listing all treasures...\n");

    DIR *dir = opendir("hunts");
    if (!dir) {
        perror("[Monitor] Failed to open hunts directory");
        fflush(stdout);
        return;
    }

    struct dirent *entry;
    int hunt_count = 0;
    int total_treasures = 0;
    
    printf("\n=== Treasures in All Hunts ===\n");
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strncmp(entry->d_name, "hunt", 4) == 0) {
            hunt_count++;
            printf("\nHunt: %s\n", entry->d_name);
            
            // Build the path to the treasures file - EXACTLY as in treasure_manager
            char file_path[512];
            snprintf(file_path, sizeof(file_path), "hunts/%s/treasures.dat", entry->d_name);
            
            // Get file information using the same approach as treasure_manager
            struct stat st;
            if (stat(file_path, &st) == -1) {
                perror("Error accessing treasure file");
                continue;
            }
            
            // Display file information in same format as treasure_manager
            printf("File size: %ld bytes\n", st.st_size);
            printf("Last modified: %s", ctime(&st.st_mtime));
            
            // Open the treasures file
            int fd = open(file_path, O_RDONLY);
            if (fd == -1) {
                perror("Error opening treasure file");
                continue;
            }
            
            // Read and display treasures in same format as treasure_manager
            printf("\nTreasures:\n");
            
            Treasure t;
            int hunt_treasures = 0;
            
            // Read treasures directly without filtering - exactly like treasure_manager
            while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
                printf("ID: %d | User: %s | Location: (%.2f, %.2f) | Value: %d\n",
                       t.id, t.username, t.latitude, t.longitude, t.value);
                printf("Clue: %s\n\n", t.clue);
                
                hunt_treasures++;
                total_treasures++;
            }
            
            close(fd);
        }
    }
    
    if (hunt_count == 0) {
        printf("No treasure hunts found.\n");
    } else {
        printf("\nTotal hunts: %d, Total treasures: %d\n", hunt_count, total_treasures);
    }
    
    closedir(dir);
    fflush(stdout);
}

void handle_list_treasures(int sig) {
    (void)sig;
    list_all_treasures();
}

// Updated to match treasure_manager view_treasure function exactly
void view_specific_treasure(const char *hunt_id, int treasure_id) {
    printf("[Monitor] Viewing treasure %d in hunt %s...\n", treasure_id, hunt_id);
    
    // Build the path to the treasures file - EXACTLY as in treasure_manager
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "hunts/%s/treasures.dat", hunt_id);
    
    // Open the treasures file
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("[Monitor] Error opening treasure file");
        fflush(stdout);
        return;
    }
    
    // Search for the specified treasure using the same approach as treasure_manager
    Treasure t;
    int found = 0;
    
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.id == treasure_id) {
            printf("\nTreasure Details:\n");
            printf("ID: %d | User: %s | Location: (%.2f, %.2f) | Value: %d\n", 
                   t.id, t.username, t.latitude, t.longitude, t.value);
            printf("Clue: %s\n", t.clue);
            found = 1;
            break;
        }
    }
    
    if (!found) {
        printf("[Monitor] Error: Treasure ID %d not found in hunt %s.\n", treasure_id, hunt_id);
    }
    
    close(fd);
    fflush(stdout);
}

void handle_view_treasure(int sig) {
    (void)sig;
    printf("[Monitor] Usage: view_treasure <hunt_id> <treasure_id>\n");
    printf("[Monitor] Available hunts:\n");
    
    DIR *dir = opendir("hunts");
    if (!dir) {
        perror("[Monitor] Failed to open hunts directory");
        fflush(stdout);
        return;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strncmp(entry->d_name, "hunt", 4) == 0) {
            printf("  - %s\n", entry->d_name);
        }
    }
    
    closedir(dir);
    fflush(stdout);
}