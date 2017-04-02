/*
 * regonf
 */
#include "main.h"

Prog * getProgById(int id, const ProgList *list) {
    LLIST_GET_BY_ID(Prog)
}

int lockProgList() {
    extern Mutex progl_mutex;
    if (pthread_mutex_lock(&(progl_mutex.self)) != 0) {
#ifdef MODE_DEBUG
        perror("lockProgList: error locking mutex");
#endif 
        return 0;
    }
    return 1;
}

int tryLockProgList() {
    extern Mutex progl_mutex;
    if (pthread_mutex_trylock(&(progl_mutex.self)) != 0) {
        return 0;
    }
    return 1;
}

int unlockProgList() {
    extern Mutex progl_mutex;
    if (pthread_mutex_unlock(&(progl_mutex.self)) != 0) {
#ifdef MODE_DEBUG
        perror("unlockProgList: error unlocking mutex (CMD_GET_ALL)");
#endif 
        return 0;
    }
    return 1;
}

int lockProg(Prog *item) {
    if (pthread_mutex_lock(&(item->mutex.self)) != 0) {
#ifdef MODE_DEBUG
        perror("lockProg: error locking mutex");
#endif 
        return 0;
    }
    return 1;
}

int tryLockProg(Prog *item) {
    if (pthread_mutex_trylock(&(item->mutex.self)) != 0) {
        return 0;
    }
    return 1;
}

int unlockProg(Prog *item) {
    if (pthread_mutex_unlock(&(item->mutex.self)) != 0) {
#ifdef MODE_DEBUG
        perror("unlockProg: error unlocking mutex (CMD_GET_ALL)");
#endif 
        return 0;
    }
    return 1;
}

int sensorRead(SensorFTS *item) {
    return acp_readSensorFTS(item, sock_buf_size);
}

int controlEM(EM *item, float output) {
    if (item == NULL) {
        return 0;
    }
    return acp_setEMDutyCycleR(item, output, sock_buf_size);
}

void secure() {
    PROG_LIST_LOOP_DF
    PROG_LIST_LOOP_ST
    controlEM(&curr->em_h, 0.0f);
    controlEM(&curr->em_c, 0.0f);
    PROG_LIST_LOOP_SP
}

int checkSensor(const SensorFTS *item) {
    if (item->source == NULL) {
        fprintf(stderr, "checkSensor: no data source where id = %d\n", item->id);
        return 0;
    }
    return 1;
}

int checkEM(const EMList *list) {
    size_t i, j;
    for (i = 0; i < list->length; i++) {
        if (list->item[i].source == NULL) {
            fprintf(stderr, "checkEm: no data source where id = %d\n", list->item[i].id);
            return 0;
        }
    }
    //unique id
    for (i = 0; i < list->length; i++) {
        for (j = i + 1; j < list->length; j++) {
            if (list->item[i].id == list->item[j].id) {
                fprintf(stderr, "checkEm: id is not unique where id = %d\n", list->item[i].id);
                return 0;
            }
        }
    }
    return 1;
}

struct timespec getTimeRestChange(const Prog *item) {
    return getTimeRestTmr(item->change_gap, item->tmr);
}

char * getStateStr(char state) {
    switch (state) {
        case OFF:
            return "OFF";
            break;
        case INIT:
            return "INIT";
            break;
        case DO:
            return "DO";
            break;
        case WAIT:
            return "WAIT";
            break;
        case REG:
            return "REG";
            break;
        case NOREG:
            return "NOREG";
            break;
        case COOLER:
            return "COOLER";
            break;
        case HEATER:
            return "HEATER";
            break;
            break;
        case DISABLE:
            return "DISABLE";
            break;
    }
    return "\0";
}

int bufCatProgRuntime(const Prog *item, char *buf, size_t buf_size) {
    char q[LINE_SIZE];
    char *state = getStateStr(item->state);
    char *state_r = getStateStr(item->state_r);
    struct timespec tm_rest = getTimeRestChange(item);
    snprintf(q, sizeof q, "%d_%s_%s_%f_%f_%ld_%f_%d\n",
            item->id,
            state,
            state_r,
            item->output_heater,
            item->output_cooler,
            tm_rest.tv_sec,
            item->sensor.value.value,
            item->sensor.value.state
            );
    if (bufCat(buf, q, buf_size) == NULL) {
        return 0;
    }
    return 1;
}

int bufCatProgInit(const Prog *item, char *buf, size_t buf_size) {
    char q[LINE_SIZE];
    struct timespec tm_rest = getTimeRestChange(item);
    snprintf(q, sizeof q, "%d_%f_%f_%ld\n",
            item->id,
            item->goal,
            item->delta,
            item->change_gap.tv_sec
            );
    if (bufCat(buf, q, buf_size) == NULL) {
        return 0;
    }
    return 1;
}

int sendStrPack(char qnf, char *cmd) {
    extern size_t sock_buf_size;
    extern Peer peer_client;
    return acp_sendStrPack(qnf, cmd, sock_buf_size, &peer_client);
}

int sendBufPack(char *buf, char qnf, char *cmd_str) {
    extern size_t sock_buf_size;
    extern Peer peer_client;
    return acp_sendBufPack(buf, qnf, cmd_str, sock_buf_size, &peer_client);
}

void sendStr(const char *s, uint8_t *crc) {
    acp_sendStr(s, crc, &peer_client);
}

void sendFooter(int8_t crc) {
    acp_sendFooter(crc, &peer_client);
}

void waitThread_ctl(char cmd) {
    thread_cmd = cmd;
    pthread_join(thread, NULL);
}

void printAll(ProgList *list, PeerList *pl) {
    char q[LINE_SIZE];
    uint8_t crc = 0;
    size_t i;
    snprintf(q, sizeof q, "CONFIG_FILE: %s\n", CONFIG_FILE);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "port: %d\n", sock_port);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "pid_path: %s\n", pid_path);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "sock_buf_size: %d\n", sock_buf_size);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "cycle_duration sec: %ld\n", cycle_duration.tv_sec);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "cycle_duration nsec: %ld\n", cycle_duration.tv_nsec);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "peer_lock_id: %s\n", peer_lock_id);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "db_data_path: %s\n", db_data_path);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "db_public_path: %s\n", db_public_path);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "app_state: %s\n", getAppState(app_state));
    sendStr(q, &crc);
    snprintf(q, sizeof q, "PID: %d\n", proc_id);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "prog_list length: %d\n", list->length);
    sendStr(q, &crc);
    sendStr("+-----------------------------------------------------------------------------------------------------------------------+\n", &crc);
    sendStr("|                                                      Program                                                          |\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    sendStr("|    id     |    goal   |   delta   | change_gap|change_rest|   state   |  state_r  | state_onf | out_heater| out_cooler|\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    PROG_LIST_LOOP_DF
    PROG_LIST_LOOP_ST
            char *state = getStateStr(curr->state);
    char *state_r = getStateStr(curr->state_r);
    char *state_onf = getStateStr(curr->state_onf);
    struct timespec tm1 = getTimeRestChange(curr);
    snprintf(q, sizeof q, "|%11d|%11.3f|%11.3f|%11ld|%11ld|%11s|%11s|%11s|%11.3f|%11.3f|\n",
            curr->id,
            curr->goal,
            curr->delta,
            curr->change_gap.tv_sec,
            tm1.tv_sec,
            state,
            state_r,
            state_onf,
            curr->output_heater,
            curr->output_cooler
            );
    sendStr(q, &crc);
    PROG_LIST_LOOP_SP
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);

    sendStr("+-------------------------------------------------------------------------------------+\n", &crc);
    sendStr("|                                       Peer                                          |\n", &crc);
    sendStr("+--------------------------------+-----------+-----------+----------------+-----------+\n", &crc);
    sendStr("|               id               |   link    | sock_port |      addr      |     fd    |\n", &crc);
    sendStr("+--------------------------------+-----------+-----------+----------------+-----------+\n", &crc);
    for (i = 0; i < pl->length; i++) {
        snprintf(q, sizeof q, "|%32s|%11p|%11u|%16u|%11d|\n",
                pl->item[i].id,
                &pl->item[i],
                pl->item[i].addr.sin_port,
                pl->item[i].addr.sin_addr.s_addr,
                *pl->item[i].fd
                );
        sendStr(q, &crc);
    }
    sendStr("+--------------------------------+-----------+-----------+----------------+-----------+\n", &crc);

    sendStr("+-----------------------------------------------------------------------------------------------------------+\n", &crc);
    sendStr("|                                                    Prog EM                                                |\n", &crc);
    sendStr("+-----------+-----------------------------------------------+-----------------------------------------------+\n", &crc);
    sendStr("|           |                   EM heater                   |                  EM cooler                    |\n", &crc);
    sendStr("+           +-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    sendStr("|  prog_id  |     id    | remote_id |  pwm_rsl  | peer_link |     id    | remote_id |  pwm_rsl  | peer_link |\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    PROG_LIST_LOOP_ST
    snprintf(q, sizeof q, "|%11d|%11d|%11d|%11f|%11p|%11d|%11d|%11f|%11p|\n",
            curr->id,

            curr->em_c.id,
            curr->em_c.remote_id,
            curr->em_c.pwm_rsl,
            curr->em_c.source,

            curr->em_h.id,
            curr->em_h.remote_id,
            curr->em_h.pwm_rsl,
            curr->em_h.source
            );
    sendStr(q, &crc);
    PROG_LIST_LOOP_SP
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    sendStr("+-----------+------------------------------------------------------------------------------+\n", &crc);
    sendStr("|    Prog   |                                   Sensor                                     |\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+------------------------------------------+\n", &crc);
    sendStr("|           |           |           |           |                   value                  |\n", &crc);
    sendStr("|           |           |           |           |-----------+-----------+-----------+------+\n", &crc);
    sendStr("|    id     |    id     | remote_id | peer_link |   temp    |    sec    |   nsec    | state|\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+------+\n", &crc);
    PROG_LIST_LOOP_ST
    snprintf(q, sizeof q, "|%11d|%11d|%11d|%11p|%11f|%11ld|%11ld|%6d|\n",
            curr->id,
            curr->sensor.id,
            curr->sensor.remote_id,
            curr->sensor.source,
            curr->sensor.value.value,
            curr->sensor.value.tm.tv_sec,
            curr->sensor.value.tm.tv_nsec,
            curr->sensor.value.state
            );
    sendStr(q, &crc);
    PROG_LIST_LOOP_SP
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+------+\n", &crc);
    sendFooter(crc);
}

void printHelp() {
    char q[LINE_SIZE];
    uint8_t crc = 0;
    sendStr("COMMAND LIST\n", &crc);
    snprintf(q, sizeof q, "%c\tput process into active mode; process will read configuration\n", ACP_CMD_APP_START);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tput process into standby mode; all running programs will be stopped\n", ACP_CMD_APP_STOP);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tfirst stop and then start process\n", ACP_CMD_APP_RESET);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tterminate process\n", ACP_CMD_APP_EXIT);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tget state of process; response: B - process is in active mode, I - process is in standby mode\n", ACP_CMD_APP_PING);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tget some variable's values; response will be packed into multiple packets\n", ACP_CMD_APP_PRINT);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tget this help; response will be packed into multiple packets\n", ACP_CMD_APP_HELP);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tload prog into RAM and start its execution; program id expected if '.' quantifier is used\n", ACP_CMD_START);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tunload program from RAM; program id expected if '.' quantifier is used\n", ACP_CMD_STOP);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tenable running program; program id expected if '.' quantifier is used\n", ACP_CMD_REGONF_PROG_ENABLE);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tdisable running program; program id expected if '.' quantifier is used\n", ACP_CMD_REGONF_PROG_DISABLE);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tunload program from RAM, after that load it; program id expected if '.' quantifier is used\n", ACP_CMD_RESET);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tswitch prog state between REG and NOREG; program id expected if '.' quantifier is used\n", ACP_CMD_REGONF_PROG_SWITCH_STATE);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tset heater power; program id expected if '.' quantifier is used\n", ACP_CMD_REGONF_PROG_SET_HEATER_POWER);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tset cooler power; program id expected if '.' quantifier is used\n", ACP_CMD_REGONF_PROG_SET_COOLER_POWER);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tget prog runtime data in format:  progId_state_stateEM_output_timeRestSecToEMSwap; program id expected if '.' quantifier is used\n", ACP_CMD_REGONF_PROG_GET_DATA_RUNTIME);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tget prog initial data in format;  progId_setPoint_mode_ONFdelta_PIDheaterKp_PIDheaterKi_PIDheaterKd_PIDcoolerKp_PIDcoolerKi_PIDcoolerKd; program id expected if '.' quantifier is used\n", ACP_CMD_REGONF_PROG_GET_DATA_INIT);
    sendStr(q, &crc);
    sendFooter(crc);
}
