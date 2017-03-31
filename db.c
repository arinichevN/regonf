/*
 * regonf
 */
#include "main.h"

int checkProg(const Prog *item, const ProgList *list) {
    if (item->sensor.source == NULL) {
        fprintf(stderr, "checkProg: no sensor attached to prog with id = %d\n", item->id);
        return 0;
    }
    /*
        if (item->em_c == NULL) {
            fprintf(stderr, "checkProg: no cooler EM attached to prog with id = %d\n", item->id);
            return 0;
        }
        if (item->em_h == NULL) {
            fprintf(stderr, "checkProg: no heater EM attached to prog with id = %d\n", item->id);
            return 0;
        }
     */
    if (item->delta < 0) {
        fprintf(stderr, "checkProg: bad delta (delta>=0 expected) where prog_id = %d\n", item->id);
        return 0;
    }
    //unique id
    if (getProgById(item->id, list) != NULL) {
        fprintf(stderr, "checkProg: prog with id = %d is already running\n", item->id);
        return 0;
    }
    return 1;
}

int addProg(Prog *item, ProgList *list) {
    if (list->length >= INT_MAX) {
#ifdef MODE_DEBUG
        fprintf(stderr, "addProg: ERROR: can not load prog with id=%d - list length exceeded\n", item->id);
#endif
        return 0;
    }
    if (list->top == NULL) {
        lockProgList();
        list->top = item;
        unlockProgList();
    } else {
        lockProg(list->last);
        list->last->next = item;
        unlockProg(list->last);
    }
    list->last = item;
    list->length++;
#ifdef MODE_DEBUG
    printf("addProg: prog with id=%d loaded\n", item->id);
#endif
    return 1;
}

void saveProgLoad(int id, int v, sqlite3 *db) {
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "update prog set load=%d where id=%d", v, id);
    if (!db_exec(db, q, 0, 0)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "saveProgLoad: query failed: %s\n", q);
#endif
    }
}

void saveProgEnable(int id, int v, const char* db_path) {
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        printfe("saveProgEnable: failed to open db: %s\n", db_path);
        return;
    }
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "update prog set enable=%d where id=%d", v, id);
    if (!db_exec(db, q, 0, 0)) {
        printfe("saveProgEnable: query failed: %s\n", q);
    }
}

void saveProgDelta(int id, float value, const char* db_path) {
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        printfe("saveProgDelta: failed to open db where id=%d\n", id);
        return;
    }
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "update prog set delta=%f where id=%d", value, id);
    if (!db_exec(db, q, 0, 0)) {
        printfe("saveProgDelta: query failed: %s\n", q);
    }
    sqlite3_close(db);
}

void saveProgGoal(int id, float value, const char* db_path) {
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        printfe("saveProgGoal: failed to open db where id=%d\n", id);
        return;
    }
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "update prog set goal=%f where id=%d", value, id);
    if (!db_exec(db, q, 0, 0)) {
        printfe("saveProgGoal: query failed: %s\n", q);
    }
    sqlite3_close(db);
}

int loadProg_callback(void *d, int argc, char **argv, char **azColName) {
    ProgData *data = d;
    Prog *item = (Prog *) malloc(sizeof *(item));
    if (item == NULL) {
        fputs("loadProg_callback: failed to allocate memory\n", stderr);
        return 0;
    }
    memset(item, 0, sizeof *item);
    int load = 0, enable = 0;
    for (int i = 0; i < argc; i++) {
        if (strcmp("id", azColName[i]) == 0) {
            item->id = atoi(argv[i]);
        } else if (strcmp("sensor_id", azColName[i]) == 0) {
            if (!config_getSensorFTS(&item->sensor, atoi(argv[i]), data->peer_list, data->db)) {
                free(item);
                return 1;
            }
        } else if (strcmp("em_heater_id", azColName[i]) == 0) {
            if (!config_getEM(&item->em_h, atoi(argv[i]), data->peer_list, data->db)) {
                free(item);
                return 1;
            }
        } else if (strcmp("em_cooler_id", azColName[i]) == 0) {
            if (!config_getEM(&item->em_c, atoi(argv[i]), data->peer_list, data->db)) {
                free(item);
                return 1;
            }
        } else if (strcmp("goal", azColName[i]) == 0) {
            item->goal = atof(argv[i]);
        } else if (strcmp("delta", azColName[i]) == 0) {
            item->delta = atof(argv[i]);
        } else if (strcmp("change_gap", azColName[i]) == 0) {
            item->change_gap.tv_nsec = 0;
            item->change_gap.tv_sec = atoi(argv[i]);
        } else if (strcmp("enable", azColName[i]) == 0) {
            enable = atoi(argv[i]);
        } else if (strcmp("load", azColName[i]) == 0) {
            load = atoi(argv[i]);
        } else {
            putse("loadProg_callback: unknown column: %s\n");
        }
    }

    if (enable) {
        item->state = INIT;
    } else {
        item->state = DISABLE;
    }

    item->next = NULL;

    if (!initMutex(&item->mutex)) {
        free(item);
        return 1;
    }
    if (!checkProg(item, data->prog_list)) {
        free(item);
        return 1;
    }
    if (!addProg(item, data->prog_list)) {
        free(item);
        return 1;
    }
    if (!load) {
        saveProgLoad(item->id, 1, data->db);
    }
    return 0;
}

int addProgById(int prog_id, ProgList *list, PeerList *pl, const char *db_path) {
    Prog *rprog = getProgById(prog_id, list);
    if (rprog != NULL) {//program is already running
#ifdef MODE_DEBUG
        fprintf(stderr, "WARNING: addProgById: program with id = %d is being controlled by program\n", rprog->id);
#endif
        return 0;
    }
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        return 0;
    }
    ProgData data = {db, pl, list};
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "select " PROG_FIELDS " from prog where id=%d", prog_id);
    if (!db_exec(db, q, loadProg_callback, (void*) &data)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "addProgById: query failed: %s\n", q);
#endif
        sqlite3_close(db);
        return 0;
    }
    sqlite3_close(db);
    return 1;
}

int deleteProgById(int id, ProgList *list, const char* db_path) {
#ifdef MODE_DEBUG
    printf("prog to delete: %d\n", id);
#endif
    Prog *prev = NULL, *curr;
    int done = 0;
    curr = list->top;
    while (curr != NULL) {
        if (curr->id == id) {
            sqlite3 *db;
            if (db_open(db_path, &db)) {
                saveProgLoad(curr->id, 0, db);
                sqlite3_close(db);
            }
            if (prev != NULL) {
                lockProg(prev);
                prev->next = curr->next;
                unlockProg(prev);
            } else {//curr=top
                lockProgList();
                list->top = curr->next;
                unlockProgList();
            }
            if (curr == list->last) {
                list->last = prev;
            }
            list->length--;
            //we will wait other threads to finish curr program and then we will free it
            lockProg(curr);
            unlockProg(curr);
            free(curr);
#ifdef MODE_DEBUG
            printf("prog with id: %d deleted from prog_list\n", id);
#endif
            done = 1;
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    return done;
}

int loadActiveProg(ProgList *list, PeerList *pl, char *db_path) {
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        return 0;
    }
    ProgData data = {db, pl, list};
    char *q = "select " PROG_FIELDS " from prog where load=1";
    if (!db_exec(db, q, loadProg_callback, (void*) &data)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "loadActiveProg: query failed: %s\n", q);
#endif
        sqlite3_close(db);
        return 0;
    }
    sqlite3_close(db);
    return 1;
}

int loadAllProg(ProgList *list, PeerList *pl, char *db_path) {
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        return 0;
    }
    ProgData data = {db, pl, list};
    char *q = "select " PROG_FIELDS " from prog";
    if (!db_exec(db, q, loadProg_callback, (void*) &data)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "loadAllProg: query failed: %s\n", q);
#endif
        sqlite3_close(db);
        return 0;
    }
    sqlite3_close(db);
    return 1;
}

int reloadProgById(int id, ProgList *list, PeerList *pl, const char* db_path) {
    if (deleteProgById(id, list, db_path)) {
        return 1;
    }
    return addProgById(id, list,pl, db_path);
}
