#ifndef TREASURE_H
#define TREASURE_H

#define MAX_USERNAME 32
#define MAX_CLUE_TEXT 256

typedef struct {
    int id;
    char username[MAX_USERNAME];
    float latitude;
    float longitude;
    char clue[MAX_CLUE_TEXT];
    int value;
} Treasure;

void print_treasure(const Treasure *t);

#endif // TREASURE_H
