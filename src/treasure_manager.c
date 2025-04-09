#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "../include/treasure.h"

void print_usage() {
    printf("Usage: treasure_manager --<operation> <args>\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "--add") == 0) {
        // TODO: Add treasure logic
    } else if (strcmp(argv[1], "--list") == 0) {
        // TODO: List treasures
    } else if (strcmp(argv[1], "--view") == 0) {
        // TODO: View specific treasure
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
