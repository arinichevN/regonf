/*
 * regonf
 */

#include "main.h"

char pid_path[LINE_SIZE];
int app_state = APP_INIT;

char db_data_path[LINE_SIZE];
char db_public_path[LINE_SIZE];

int pid_file = -1;
int proc_id;
int sock_port = -1;
int sock_fd = -1;
int sock_fd_tf = -1;
Peer peer_client = {.fd = &sock_fd, .addr_size = sizeof peer_client.addr};
struct timespec cycle_duration = {0, 0};
DEF_THREAD
Mutex progl_mutex = {.created = 0, .attr_initialized = 0};
I1List i1l = {NULL, 0};
I2List i2l = {NULL, 0};
I1S1List i1s1l = {NULL, 0};
I1F1List i1f1l = {NULL, 0};
PeerList peer_list = {NULL, 0};
ProgList prog_list = {NULL, NULL, 0};

#include "util.c"
#include "db.c"

int readSettings() {
#ifdef MODE_DEBUG
    printf("readSettings: configuration file to read: %s\n", CONFIG_FILE);
#endif
    FILE* stream = fopen(CONFIG_FILE, "r");
    if (stream == NULL) {
#ifdef MODE_DEBUG
        perror("readSettings()");
#endif
        return 0;
    }
    skipLine(stream);
    int n;
    n = fscanf(stream, "%d\t%255s\t%ld\t%ld\t%255s\t%255s\n",
            &sock_port,
            pid_path,
            &cycle_duration.tv_sec,
            &cycle_duration.tv_nsec,
            db_data_path,
            db_public_path
            );
    if (n != 6) {
        fclose(stream);
        return 0;
    }
    fclose(stream);
    return 1;
}

void initApp() {
    if (!readSettings()) {
        exit_nicely_e("initApp: failed to read settings\n");
    }
    if (!initPid(&pid_file, &proc_id, pid_path)) {
        exit_nicely_e("initApp: failed to initialize pid\n");
    }
#ifdef MODE_DEBUG
    printf("initApp: CONFIG_FILE: %s\n", CONFIG_FILE);
    printf("initApp: PID: %d\n", proc_id);
    printf("initApp: sock_port: %d\n", sock_port);
    printf("initApp: pid_path: %s\n", pid_path);
    printf("initApp: cycle_duration: %ld(sec) %ld(nsec)\n", cycle_duration.tv_sec, cycle_duration.tv_nsec);
    printf("initApp: db_data_path: %s\n", db_data_path);
    printf("initApp: db_public_path: %s\n", db_public_path);
#endif
    if (!initMutex(&progl_mutex)) {
        exit_nicely_e("initApp: failed to initialize prog mutex\n");
    }

    if (!initServer(&sock_fd, sock_port)) {
        exit_nicely_e("initApp: failed to initialize udp server\n");
    }

    if (!initClient(&sock_fd_tf, WAIT_RESP_TIMEOUT)) {
        exit_nicely_e("initApp: failed to initialize udp client\n");
    }
}

int initData() {
    if (!config_getPeerList(&peer_list, &sock_fd_tf, db_public_path)) {
        FREE_LIST(&peer_list);
        return 0;
    }
    if (!loadActiveProg(&prog_list, &peer_list, db_data_path)) {
#ifdef MODE_DEBUG
        fputs("initData: ERROR: failed to load active programs\n", stderr);
#endif
        freeProg(&prog_list);
        FREE_LIST(&peer_list);
        return 0;
    }
    if (!initI1List(&i1l, ACP_BUFFER_MAX_SIZE)) {
#ifdef MODE_DEBUG
        fputs("initData: ERROR: failed to allocate memory for i1l\n", stderr);
#endif
        freeProg(&prog_list);
        FREE_LIST(&peer_list);
        return 0;
    }
    if (!initI1F1List(&i1f1l, ACP_BUFFER_MAX_SIZE)) {
#ifdef MODE_DEBUG
        fputs("initData: ERROR: failed to allocate memory for i1f1l\n", stderr);
#endif
        FREE_LIST(&i1l);
        freeProg(&prog_list);
        FREE_LIST(&peer_list);
        return 0;
    }
    if (!initI2List(&i2l, ACP_BUFFER_MAX_SIZE)) {
#ifdef MODE_DEBUG
        fputs("initData: ERROR: failed to allocate memory for i2l\n", stderr);
#endif
        FREE_LIST(&i1f1l);
        FREE_LIST(&i1l);
        freeProg(&prog_list);
        FREE_LIST(&peer_list);
        return 0;
    }
    if (!initI1S1List(&i1s1l, ACP_BUFFER_MAX_SIZE)) {
#ifdef MODE_DEBUG
        fputs("initData: ERROR: failed to allocate memory for i1s1l\n", stderr);
#endif
        FREE_LIST(&i2l);
        FREE_LIST(&i1f1l);
        FREE_LIST(&i1l);
        freeProg(&prog_list);
        FREE_LIST(&peer_list);
        return 0;
    }
    if (!THREAD_CREATE) {
#ifdef MODE_DEBUG
        fputs("initData: ERROR: failed to create thread\n", stderr);
#endif
        FREE_LIST(&i1s1l);
        FREE_LIST(&i2l);
        FREE_LIST(&i1f1l);
        FREE_LIST(&i1l);
        freeProg(&prog_list);
        FREE_LIST(&peer_list);
        return 0;
    }
    return 1;
}
#define PARSE_I1LIST acp_requestDataToI1List(&request, &i1l, prog_list.length);if (i1l.length <= 0) {return;}
#define PARSE_I1F1LIST acp_requestDataToI1F1List(&request, &i1f1l, prog_list.length);if (i1f1l.length <= 0) {return;}
#define PARSE_I2LIST acp_requestDataToI2List(&request, &i2l, prog_list.length);if (i2l.length <= 0) {return;}
#define PARSE_I1S1LIST acp_requestDataToI1S1List(&request, &i1s1l, prog_list.length);if (i1s1l.length <= 0) {return;}

void serverRun(int *state, int init_state) {
    SERVER_HEADER
    SERVER_APP_ACTIONS

    if (ACP_CMD_IS(ACP_CMD_PROG_STOP)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *curr = getProgById(i1l.item[i], &prog_list);
            if (curr != NULL) {
                regonfhc_turnOff(&curr->reg);
                deleteProgById(i1l.item[i], &prog_list, db_data_path);
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_START)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            addProgById(i1l.item[i], &prog_list, &peer_list, db_data_path);
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_RESET)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *curr = getProgById(i1l.item[i], &prog_list);
            if (curr != NULL) {
                regonfhc_turnOff(&curr->reg);
                deleteProgById(i1l.item[i], &prog_list, db_data_path);
            }
        }
        for (int i = 0; i < i1l.length; i++) {
            addProgById(i1l.item[i], &prog_list, &peer_list, db_data_path);
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_ENABLE)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *curr = getProgById(i1l.item[i], &prog_list);
            if (curr != NULL) {
                if (lockProg(curr)) {
                    regonfhc_enable(&curr->reg);
                    saveProgEnable(curr->id, 1, db_data_path);
                    unlockProg(curr);
                }
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_DISABLE)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *curr = getProgById(i1l.item[i], &prog_list);
            if (curr != NULL) {
                if (lockProg(curr)) {
                    regonfhc_disable(&curr->reg);
                    saveProgEnable(curr->id, 0, db_data_path);
                    unlockProg(curr);
                }
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_GET_DATA_RUNTIME)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *curr = getProgById(i1l.item[i], &prog_list);
            if (curr != NULL) {
                if (!bufCatProgRuntime(curr, &response)) {
                    return;
                }
            }
        }
    } else if (ACP_CMD_IS(ACP_CMD_PROG_GET_DATA_INIT)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *curr = getProgById(i1l.item[i], &prog_list);
            if (curr != NULL) {
                if (!bufCatProgInit(curr, &response)) {
                    return;
                }
            }
        }
    } else if (ACP_CMD_IS(ACP_CMD_PROG_GET_ENABLED)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *curr = getProgById(i1l.item[i], &prog_list);
            if (curr != NULL) {
                if (!bufCatProgEnabled(curr, &response)) {
                    return;
                }
            }
        }
    } else if (ACP_CMD_IS(ACP_CMD_REG_PROG_GET_GOAL)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *curr = getProgById(i1l.item[i], &prog_list);
            if (curr != NULL) {
                if (!bufCatProgGoal(curr, &response)) {
                    return;
                }
            }
        }
    } else if (ACP_CMD_IS(ACP_CMD_GET_FTS)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *curr = getProgById(i1l.item[i], &prog_list);
            if (curr != NULL) {
                if (!bufCatProgFTS(curr, &response)) {
                    return;
                }
            }
        }
    } else if (ACP_CMD_IS(ACP_CMD_REG_PROG_SET_HEATER_POWER)) {
        PARSE_I1F1LIST
        for (int i = 0; i < i1f1l.length; i++) {
            Prog *curr = getProgById(i1f1l.item[i].p0, &prog_list);
            if (curr != NULL) {
                if (lockProg(curr)) {
                    regonfhc_setHeaterPower(&curr->reg, i1f1l.item[i].p1);
                    unlockProg(curr);
                }
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_REG_PROG_SET_COOLER_POWER)) {
        PARSE_I1F1LIST
        for (int i = 0; i < i1f1l.length; i++) {
            Prog *curr = getProgById(i1f1l.item[i].p0, &prog_list);
            if (curr != NULL) {
                if (lockProg(curr)) {
                    regonfhc_setCoolerPower(&curr->reg, i1f1l.item[i].p1);
                    unlockProg(curr);
                }
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_REG_PROG_SET_GOAL)) {
        PARSE_I1F1LIST
        for (int i = 0; i < i1f1l.length; i++) {
            Prog *curr = getProgById(i1f1l.item[i].p0, &prog_list);
            if (curr != NULL) {
                if (lockProg(curr)) {
                    regonfhc_setGoal(&curr->reg, i1f1l.item[i].p1);
                    unlockProg(curr);
                }
            }
            saveProgFieldFloat(i1f1l.item[i].p0, i1f1l.item[i].p1, db_data_path, "goal");
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_REGONF_PROG_SET_HEATER_DELTA)) {
        PARSE_I1F1LIST
        for (int i = 0; i < i1f1l.length; i++) {
            Prog *curr = getProgById(i1f1l.item[i].p0, &prog_list);
            if (curr != NULL) {
                if (lockProg(curr)) {
                    regonfhc_setHeaterDelta(&curr->reg, i1f1l.item[i].p1);
                    unlockProg(curr);
                }
            }
            saveProgFieldFloat(i1f1l.item[i].p0, i1f1l.item[i].p1, db_data_path, "heater_delta");
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_REGONF_PROG_SET_COOLER_DELTA)) {
        PARSE_I1F1LIST
        for (int i = 0; i < i1f1l.length; i++) {
            Prog *curr = getProgById(i1f1l.item[i].p0, &prog_list);
            if (curr != NULL) {
                if (lockProg(curr)) {
                    regonfhc_setCoolerDelta(&curr->reg, i1f1l.item[i].p1);
                    unlockProg(curr);
                }
            }
            saveProgFieldFloat(i1f1l.item[i].p0, i1f1l.item[i].p1, db_data_path, "cooler_delta");
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_REG_PROG_SET_CHANGE_GAP)) {
        PARSE_I2LIST
        for (int i = 0; i < i2l.length; i++) {
            Prog *curr = getProgById(i2l.item[i].p0, &prog_list);
            if (curr != NULL) {
                if (lockProg(curr)) {
                    regonfhc_setChangeGap(&curr->reg, i2l.item[i].p1);
                    unlockProg(curr);
                }
            }
            saveProgFieldInt(i2l.item[i].p0, i2l.item[i].p1, db_data_path, "change_gap");
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_REG_PROG_SET_EM_MODE)) {
        PARSE_I1S1LIST
        for (int i = 0; i < i1s1l.length; i++) {
            Prog *curr = getProgById(i1s1l.item[i].p0, &prog_list);
            if (curr != NULL) {
                if (lockProg(curr)) {
                    regonfhc_setEMMode(&curr->reg, i1s1l.item[i].p1);
                    unlockProg(curr);
                }
            }
            saveProgFieldText(i1s1l.item[i].p0, i1s1l.item[i].p1, db_data_path, "em_mode");
        }
        return;

    }
    acp_responseSend(&response, &peer_client);
}

void progControl(Prog *item) {
#ifdef MODE_DEBUG
    printf("progId: %d ", item->id);
#endif
    regonfhc_control(&item->reg);
}

void *threadFunction(void *arg) {
    THREAD_DEF_CMD
#ifdef MODE_DEBUG
            puts("threadFunction: running...");
#endif
    while (1) {
        struct timespec t1 = getCurrentTime();

        lockProgList();
        Prog *curr = prog_list.top;
        unlockProgList();
        while (1) {
            if (curr == NULL) {
                break;
            }
            if (tryLockProg(curr)) {
                progControl(curr);
                Prog *temp = curr;
                curr = curr->next;
                unlockProg(temp);
            }


            THREAD_EXIT_ON_CMD
        }
        THREAD_EXIT_ON_CMD
        sleepRest(cycle_duration, t1);
    }
}

void freeProg(ProgList * list) {
    Prog *curr = list->top, *temp;
    while (curr != NULL) {
        temp = curr;
        curr = curr->next;
        free(temp);
    }
    list->top = NULL;
    list->last = NULL;
    list->length = 0;
}

void freeData() {
    THREAD_STOP
    secure();
    freeProg(&prog_list);
    FREE_LIST(&i1s1l);
    FREE_LIST(&i2l);
    FREE_LIST(&i1f1l);
    FREE_LIST(&i1l);
    FREE_LIST(&peer_list);
#ifdef MODE_DEBUG
    puts("freeData: done");
#endif
}

void freeApp() {
    freeData();
    freeSocketFd(&sock_fd);
    freeSocketFd(&sock_fd_tf);
    freePid(&pid_file, &proc_id, pid_path);
}

void exit_nicely() {
    freeApp();
#ifdef MODE_DEBUG
    puts("\nBye...");
#endif
    exit(EXIT_SUCCESS);
}

void exit_nicely_e(char *s) {
    fprintf(stderr, "%s", s);
    freeApp();
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
    if (geteuid() != 0) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s: root user expected\n", APP_NAME_STR);
#endif
        return (EXIT_FAILURE);
    }
#ifndef MODE_DEBUG
    daemon(0, 0);
#endif
    conSig(&exit_nicely);
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        perror("main: memory locking failed");
    }
    int data_initialized = 0;
    while (1) {
        switch (app_state) {
            case APP_INIT:
#ifdef MODE_DEBUG
                puts("MAIN: init");
#endif
                initApp();
                app_state = APP_INIT_DATA;
                break;
            case APP_INIT_DATA:
#ifdef MODE_DEBUG
                puts("MAIN: init data");
#endif
                data_initialized = initData();
                app_state = APP_RUN;
                delayUsIdle(1000000);
                break;
            case APP_RUN:
#ifdef MODE_DEBUG
                puts("MAIN: run");
#endif
                serverRun(&app_state, data_initialized);
                break;
            case APP_STOP:
#ifdef MODE_DEBUG
                puts("MAIN: stop");
#endif
                freeData();
                data_initialized = 0;
                app_state = APP_RUN;
                break;
            case APP_RESET:
#ifdef MODE_DEBUG
                puts("MAIN: reset");
#endif
                freeApp();
                delayUsIdle(1000000);
                data_initialized = 0;
                app_state = APP_INIT;
                break;
            case APP_EXIT:
#ifdef MODE_DEBUG
                puts("MAIN: exit");
#endif
                exit_nicely();
                break;
            default:
                exit_nicely_e("main: unknown application state");
                break;
        }
    }
    freeApp();
    return (EXIT_SUCCESS);
}