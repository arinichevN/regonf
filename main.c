#include "main.h"

int app_state = APP_INIT;

TSVresult config_tsv = TSVRESULT_INITIALIZER;
char * db_data_path;

int sock_port = -1;
int sock_fd = -1;

Peer peer_client = {.fd = &sock_fd, .addr_size = sizeof peer_client.addr};
struct timespec cycle_duration = {0, 0};
Mutex progl_mutex = MUTEX_INITIALIZER;

PeerList peer_list = LIST_INITIALIZER;
SensorFTSList sensor_list = LIST_INITIALIZER;
EMList em_list = LIST_INITIALIZER;
ProgList prog_list = LLIST_INITIALIZER;

#include "util.c"
#include "db.c"

int readSettings(TSVresult* r, const char *data_path, int *port, struct timespec *cd, char **db_data_path) {
    if (!TSVinit(r, data_path)) {
        return 0;
    }
    int _port = TSVgetis(r, 0, "port");
    int _cd_sec = TSVgetis(r, 0, "cd_sec");
    int _cd_nsec = TSVgetis(r, 0, "cd_nsec");
    char *_db_data_path = TSVgetvalues(r, 0, "db_data_path");
    if (TSVnullreturned(r)) {
        return 0;
    }
    *port = _port;
    cd->tv_sec = _cd_sec;
    cd->tv_nsec = _cd_nsec;
    *db_data_path = _db_data_path;
    return 1;
}

void initApp() {
    if (!readSettings(&config_tsv, CONFIG_FILE, &sock_port, &cycle_duration, &db_data_path)) {
        exit_nicely_e("initApp: failed to read settings\n");
    }
#ifdef MODE_DEBUG
    printf("%s(): \n\tsock_port: %d, \n\tcycle_duration: %ld sec %ld nsec, \n\tdb_data_path: %s\n", F, sock_port, cycle_duration.tv_sec, cycle_duration.tv_nsec, db_data_path);
#endif
    if (!initMutex(&progl_mutex)) {
        exit_nicely_e("initApp: failed to initialize prog mutex\n");
    }
    if (!initServer(&sock_fd, sock_port)) {
        exit_nicely_e("initApp: failed to initialize udp server\n");
    }
}

int initData() {
    if (!config_getPeerList(&peer_list, NULL, db_data_path)) {
        return 0;
    }
    if (!config_getSensorFTSList(&sensor_list, &peer_list, db_data_path)) {
        freePeerList(&peer_list);
        return 0;
    }
    if (!config_getEMList(&em_list, &peer_list, db_data_path)) {
        FREE_LIST(&sensor_list);
        freePeerList(&peer_list);
        return 0;
    }
    if (!loadActiveProg(&prog_list, &em_list, &sensor_list, db_data_path)) {
        freeProgList(&prog_list);
        FREE_LIST(&em_list);
        FREE_LIST(&sensor_list);
        freePeerList(&peer_list);
        return 0;
    }
    return 1;
}
#define PARSE_I1LIST acp_requestDataToI1List(&request, &i1l);if (i1l.length <= 0) {return;}
#define PARSE_I1F1LIST acp_requestDataToI1F1List(&request, &i1f1l);if (i1f1l.length <= 0) {return;}
#define PARSE_I2LIST acp_requestDataToI2List(&request, &i2l);if (i2l.length <= 0) {return;}
#define PARSE_I1S1LIST acp_requestDataToI1S1List(&request, &i1s1l);if (i1s1l.length <= 0) {return;}

void serverRun(int *state, int init_state) {
    SERVER_HEADER
    SERVER_APP_ACTIONS
    DEF_SERVER_I1LIST
    DEF_SERVER_I2LIST
    DEF_SERVER_I1F1LIST
    DEF_SERVER_I1S1LIST
    if (ACP_CMD_IS(ACP_CMD_PROG_STOP)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *item = getProgById(i1l.item[i], &prog_list);
            if (item != NULL) {
                if (lockMutex(&item->mutex)) {
                    regonfhc_turnOff(&item->reg);
                    unlockMutex(&item->mutex);
                }
                deleteProgById(i1l.item[i], &prog_list, db_data_path);
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_START)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            addProgById(i1l.item[i], &prog_list, &em_list, &sensor_list, NULL, db_data_path);
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_RESET)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *item = getProgById(i1l.item[i], &prog_list);
            if (item != NULL) {
                if (lockMutex(&item->mutex)) {
                    regonfhc_turnOff(&item->reg);
                    unlockMutex(&item->mutex);
                }
                deleteProgById(i1l.item[i], &prog_list, db_data_path);
            }
        }
        for (int i = 0; i < i1l.length; i++) {
            addProgById(i1l.item[i], &prog_list, &em_list, &sensor_list, NULL, db_data_path);
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_ENABLE)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *item = getProgById(i1l.item[i], &prog_list);
            if (item != NULL) {
                if (lockMutex(&item->mutex)) {
                    regonfhc_enable(&item->reg);
                    if (item->save)db_saveTableFieldInt("prog", "enable", item->id, 1, NULL, db_data_path);
                    unlockMutex(&item->mutex);
                }
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_DISABLE)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *item = getProgById(i1l.item[i], &prog_list);
            if (item != NULL) {
                if (lockMutex(&item->mutex)) {
                    regonfhc_disable(&item->reg);
                    if (item->save)db_saveTableFieldInt("prog", "enable", item->id, 0, NULL, db_data_path);
                    unlockMutex(&item->mutex);
                }
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_SET_SAVE)) {
        PARSE_I2LIST
        for (int i = 0; i < i2l.length; i++) {
            Prog *item = getProgById(i2l.item[i].p0, &prog_list);
            if (item != NULL) {
                if (lockMutex(&item->mutex)) {
                    item->save = i2l.item[i].p1;
                    if (item->save)db_saveTableFieldInt("prog", "save", item->id, i2l.item[i].p1, NULL, db_data_path);
                    unlockMutex(&item->mutex);
                }
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_GET_DATA_RUNTIME)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *item = getProgById(i1l.item[i], &prog_list);
            if (item != NULL) {
                if (!bufCatProgRuntime(item, &response)) {
                    return;
                }
            }
        }
    } else if (ACP_CMD_IS(ACP_CMD_PROG_GET_DATA_INIT)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *item = getProgById(i1l.item[i], &prog_list);
            if (item != NULL) {
                if (!bufCatProgInit(item, &response)) {
                    return;
                }
            }
        }
    } else if (ACP_CMD_IS(ACP_CMD_PROG_GET_ENABLED)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *item = getProgById(i1l.item[i], &prog_list);
            if (item != NULL) {
                if (!bufCatProgEnabled(item, &response)) {
                    return;
                }
            }
        }
    } else if (ACP_CMD_IS(ACP_CMD_REG_PROG_GET_GOAL)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *item = getProgById(i1l.item[i], &prog_list);
            if (item != NULL) {
                if (!bufCatProgGoal(item, &response)) {
                    return;
                }
            }
        }
    } else if (ACP_CMD_IS(ACP_CMD_GET_FTS)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *item = getProgById(i1l.item[i], &prog_list);
            if (item != NULL) {
                if (!bufCatProgFTS(item, &response)) {
                    return;
                }
            }
        }
    } else if (ACP_CMD_IS(ACP_CMD_REG_PROG_SET_HEATER_POWER)) {
        PARSE_I1F1LIST
        for (int i = 0; i < i1f1l.length; i++) {
            Prog *item = getProgById(i1f1l.item[i].p0, &prog_list);
            if (item != NULL) {
                if (lockMutex(&item->mutex)) {
                    regonfhc_setHeaterPower(&item->reg, i1f1l.item[i].p1);
                    unlockMutex(&item->mutex);
                }
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_REG_PROG_SET_COOLER_POWER)) {
        PARSE_I1F1LIST
        for (int i = 0; i < i1f1l.length; i++) {
            Prog *item = getProgById(i1f1l.item[i].p0, &prog_list);
            if (item != NULL) {
                if (lockMutex(&item->mutex)) {
                    regonfhc_setCoolerPower(&item->reg, i1f1l.item[i].p1);
                    unlockMutex(&item->mutex);
                }
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_REG_PROG_SET_GOAL)) {
        PARSE_I1F1LIST
        for (int i = 0; i < i1f1l.length; i++) {
            Prog *item = getProgById(i1f1l.item[i].p0, &prog_list);
            if (item != NULL) {
                if (lockMutex(&item->mutex)) {
                    regonfhc_setGoal(&item->reg, i1f1l.item[i].p1);
                    regonfhc_secureOutTouch(&item->reg);
                    if (item->save)db_saveTableFieldFloat("prog", "goal", i1f1l.item[i].p0, i1f1l.item[i].p1, NULL, db_data_path);
                    unlockMutex(&item->mutex);
                }

            }

        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_REGONF_PROG_SET_HEATER_DELTA)) {
        PARSE_I1F1LIST
        for (int i = 0; i < i1f1l.length; i++) {
            Prog *item = getProgById(i1f1l.item[i].p0, &prog_list);
            if (item != NULL) {
                if (lockMutex(&item->mutex)) {
                    regonfhc_setHeaterDelta(&item->reg, i1f1l.item[i].p1);
                    if (item->save)db_saveTableFieldFloat("prog", "heater_delta", i1f1l.item[i].p0, i1f1l.item[i].p1, NULL, db_data_path);
                    unlockMutex(&item->mutex);
                }

            }

        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_REGONF_PROG_SET_COOLER_DELTA)) {
        PARSE_I1F1LIST
        for (int i = 0; i < i1f1l.length; i++) {
            Prog *item = getProgById(i1f1l.item[i].p0, &prog_list);
            if (item != NULL) {
                if (lockMutex(&item->mutex)) {
                    regonfhc_setCoolerDelta(&item->reg, i1f1l.item[i].p1);
                    if (item->save)db_saveTableFieldFloat("prog", "cooler_delta", i1f1l.item[i].p0, i1f1l.item[i].p1, NULL, db_data_path);
                    unlockMutex(&item->mutex);
                }
            }

        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_REG_PROG_SET_CHANGE_GAP)) {
        PARSE_I2LIST
        for (int i = 0; i < i2l.length; i++) {
            Prog *item = getProgById(i2l.item[i].p0, &prog_list);
            if (item != NULL) {
                if (lockMutex(&item->mutex)) {
                    regonfhc_setChangeGap(&item->reg, i2l.item[i].p1);
                    if (item->save)db_saveTableFieldInt("prog", "change_gap", i2l.item[i].p0, i2l.item[i].p1, NULL, db_data_path);
                    unlockMutex(&item->mutex);
                }
            }

        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_REG_PROG_SET_EM_MODE)) {
        PARSE_I1S1LIST
        for (int i = 0; i < i1s1l.length; i++) {
            Prog *item = getProgById(i1s1l.item[i].p0, &prog_list);
            if (item != NULL) {
                if (lockMutex(&item->mutex)) {
                    regonfhc_setEMMode(&item->reg, i1s1l.item[i].p1);
                    if (item->save)db_saveTableFieldText("prog", "em_mode", i1s1l.item[i].p0, i1s1l.item[i].p1, NULL, db_data_path);
                    unlockMutex(&item->mutex);
                }
            }

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

void cleanup_handler(void *arg) {
    Prog *item = arg;
    printf("cleaning up thread %d\n", item->id);
}

void *threadFunction(void *arg) {
    Prog *item = arg;
#ifdef MODE_DEBUG
    printf("thread for program with id=%d has been started\n", item->id);
#endif
#ifdef MODE_DEBUG
    pthread_cleanup_push(cleanup_handler, item);
#endif
    while (1) {
        struct timespec t1 = getCurrentTime();
        int old_state;
        if (threadCancelDisable(&old_state)) {
            if (lockMutex(&item->mutex)) {
                progControl(item);
                unlockMutex(&item->mutex);
            }
            threadSetCancelState(old_state);
        }
        sleepRest(item->cycle_duration, t1);
    }
#ifdef MODE_DEBUG
    pthread_cleanup_pop(1);
#endif
}

void freeData() {
    stopAllProgThreads(&prog_list);
    secure();
    freeProgList(&prog_list);
    FREE_LIST(&em_list);
    FREE_LIST(&sensor_list);
    freePeerList(&peer_list);
}

void freeApp() {
    freeData();
    freeSocketFd(&sock_fd);
    freeMutex(&progl_mutex);
    TSVclear(&config_tsv);
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
#ifndef MODE_DEBUG
    daemon(0, 0);
#endif
    conSig(&exit_nicely);
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        perror("main: memory locking failed");
    }
    int data_initialized = 0;
    while (1) {
#ifdef MODE_DEBUG
        printf("%s(): %s %d\n", F, getAppState(app_state), data_initialized);
#endif
        switch (app_state) {
            case APP_INIT:
                initApp();
                app_state = APP_INIT_DATA;
                break;
            case APP_INIT_DATA:
                data_initialized = initData();
                app_state = APP_RUN;
                delayUsIdle(1000000);
                break;
            case APP_RUN:
                serverRun(&app_state, data_initialized);
                break;
            case APP_STOP:
                freeData();
                data_initialized = 0;
                app_state = APP_RUN;
                break;
            case APP_RESET:
                freeApp();
                delayUsIdle(1000000);
                data_initialized = 0;
                app_state = APP_INIT;
                break;
            case APP_EXIT:
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