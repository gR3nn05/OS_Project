#ifndef MONITOR_H
#define MONITOR_H

typedef struct {
    int id;
    char username[128];
    float latitude;
    float longitude;
    char clue[128];
    int value;
} Treasure;

void run_monitor();
void handle_list_hunts(int sig);
void handle_list_treasures(int sig);
void handle_view_treasure(int sig);
void view_specific_treasure(const char *hunt_id, int treasure_id);
void list_all_treasures();

#endif // MONITOR_H