#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/select.h>
#include "monitor.h"

pid_t monitor_pid = -1;
int pipe_FP[2]; // Pipe for father-to-process communication (commands)
int pipe_FS[2]; // Pipe for process-to-father communication (responses)
int monitor_terminating = 0; // Flag to indicate monitor is in the process of terminating

void run_hub() {
    char command[128];
    char response[4096]; // Larger buffer for responses

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
            
            if (monitor_terminating) {
                printf("Monitor is terminating, please wait...\n");
                continue;
            }

            // Create two pipes
            if (pipe(pipe_FP) == -1 || pipe(pipe_FS) == -1) {
                perror("pipe");
                continue;
            }

            monitor_pid = fork();
            if (monitor_pid == 0) {
                // Child process: Run monitor
                close(pipe_FP[1]); // Close write end of father-to-process pipe
                close(pipe_FS[0]); // Close read end of process-to-father pipe

                // Redirect stdin to read from pipe_FP
                dup2(pipe_FP[0], STDIN_FILENO);
                
                // Redirect stdout to write to pipe_FS
                dup2(pipe_FS[1], STDOUT_FILENO);
                
                // Close original file descriptors
                close(pipe_FP[0]);
                close(pipe_FS[1]);
                
                // Execute the monitor function
                run_monitor();
                exit(0);
            } else if (monitor_pid > 0) {
                // Parent process
                close(pipe_FP[0]); // Close read end of father-to-process pipe
                close(pipe_FS[1]); // Close write end of process-to-father pipe
                
                printf("Started monitor with PID %d\n", monitor_pid);
                
                // Get initial message from monitor
                fd_set readfds;
                struct timeval tv;
                
                FD_ZERO(&readfds);
                FD_SET(pipe_FS[0], &readfds);
                
                // Set timeout to 1 second
                tv.tv_sec = 1;
                tv.tv_usec = 0;
                
                int ready = select(pipe_FS[0] + 1, &readfds, NULL, NULL, &tv);
                
                if (ready > 0) {
                    ssize_t bytes_read = read(pipe_FS[0], response, sizeof(response) - 1);
                    if (bytes_read > 0) {
                        response[bytes_read] = '\0';
                        printf("%s", response);
                    }
                }
            } else {
                perror("fork");
            }
        }

        else if (monitor_pid > 0 && strcmp(command, "stop_monitor") == 0) {
            if (monitor_terminating) {
                printf("Monitor is already terminating, please wait...\n");
                continue;
            }
            
            monitor_terminating = 1;
            kill(monitor_pid, SIGTERM);
            printf("Stopping monitor...\n");
            
            // Wait for monitor to exit with a timeout
            int status;
            pid_t result;
            
            // Try to wait for the process for a limited time
            int timeout = 5; // 5 seconds
            while (timeout > 0) {
                result = waitpid(monitor_pid, &status, WNOHANG);
                if (result == monitor_pid) {
                    // Monitor has exited
                    printf("[Hub] Monitor exited with status %d\n", WEXITSTATUS(status));
                    monitor_pid = -1;
                    monitor_terminating = 0;
                    
                    // Close pipes
                    close(pipe_FP[1]);
                    close(pipe_FS[0]);
                    break;
                } else if (result == 0) {
                    // Monitor is still running
                    usleep(100000); // Wait 0.1 second
                    timeout--;
                }else {
            // Error or monitor already handled by signal handler
                     if (monitor_pid == -1) {
                // Already handled by signal handler
                    break;
                } else {
                    perror("waitpid");
                    break;
            }
        }
    }
            if (timeout == 0 && monitor_pid != -1) {
                printf("Monitor is taking longer than expected to terminate...\n");
            }
        }

        else if (monitor_pid > 0 && strcmp(command, "exit") == 0) {
            printf("Error: Monitor is still running. Use stop_monitor first.\n");
        }

        else if (strcmp(command, "exit") == 0) {
            break;
        }

        else if (monitor_pid > 0) {
            if (monitor_terminating) {
                printf("Monitor is terminating, command ignored.\n");
                continue;
            }
            
            // Send command to monitor
            write(pipe_FP[1], command, strlen(command));
            write(pipe_FP[1], "\n", 1); // Add newline
            
            // Read response from monitor with timeout
            fd_set readfds;
            struct timeval tv;
            
            // Read response in chunks until no more data
            ssize_t total_bytes_read = 0;
            ssize_t bytes_read = 0;
            response[0] = '\0'; // Initialize empty string
            
            do {
                FD_ZERO(&readfds);
                FD_SET(pipe_FS[0], &readfds);
                
                // Set timeout to 1 second for first read, shorter for subsequent reads
                tv.tv_sec = (total_bytes_read == 0) ? 1 : 0;
                tv.tv_usec = (total_bytes_read == 0) ? 0 : 100000; // 100ms for subsequent reads
                
                int ready = select(pipe_FS[0] + 1, &readfds, NULL, NULL, &tv);
                
                if (ready > 0) {
                    bytes_read = read(pipe_FS[0], response + total_bytes_read, 
                                     sizeof(response) - 1 - total_bytes_read);
                    
                    if (bytes_read > 0) {
                        total_bytes_read += bytes_read;
                        response[total_bytes_read] = '\0';
                    }
                } else {
                    // No more data or timeout
                    break;
                }
            } while (bytes_read > 0 && (size_t)total_bytes_read < sizeof(response) - 1);
            
            if (total_bytes_read > 0) {
                printf("%s", response);
            } else {
                printf("No response from monitor (timeout).\n");
            }
        }

        else if (strcmp(command, "help") == 0) {
            printf("Available commands:\n");
            printf("  start_monitor   - Start the monitor process\n");
            printf("  list_hunts      - List all treasure hunts\n");
            printf("  list_treasures  - List all treasures in all hunts\n");
            printf("  view_treasure <hunt_id> <treasure_id> - View details of a specific treasure\n");
            printf("  stop_monitor    - Stop the monitor process\n");
            printf("  exit            - Exit the program (monitor must be stopped first)\n");
            printf("  help            - Show this help message\n");
        }
        else {
            printf("Unknown command: %s\n", command);
        }
    }
}

void sigchld_handler(int sig) {
    (void)sig;
    int status;
    pid_t pid = waitpid(monitor_pid, &status, WNOHANG);
    
    if (pid == monitor_pid) {
        printf("[Hub] Monitor exited with status %d\n", WEXITSTATUS(status));
        monitor_pid = -1;
        monitor_terminating = 0;
        
        // Close pipes
        close(pipe_FP[1]);
        close(pipe_FS[0]);
    }
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