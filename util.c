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

void secure() {
    PROG_LIST_LOOP_DF
    PROG_LIST_LOOP_ST
    regonfhc_turnOff(&curr->reg);
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
    return getTimeRestTmr(item->reg.change_gap, item->reg.tmr);
}

int bufCatProgRuntime(const Prog *item, char *buf, size_t buf_size) {
    char q[LINE_SIZE];
    char *state = reg_getStateStr(item->reg.state);
    char *state_r = reg_getStateStr(item->reg.state_r);
    struct timespec tm_rest = getTimeRestChange(item);
    snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%s" ACP_DELIMITER_COLUMN_STR "%s" ACP_DELIMITER_COLUMN_STR FLOAT_NUM ACP_DELIMITER_COLUMN_STR FLOAT_NUM ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR FLOAT_NUM ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_ROW_STR,
            item->id,
            state,
            state_r,
            item->reg.heater.output,
            item->reg.cooler.output,
            tm_rest.tv_sec,
            item->reg.sensor.value.value,
            item->reg.sensor.value.state
            );
    if (bufCat(buf, q, buf_size) == NULL) {
        return 0;
    }
    return 1;
}

int bufCatProgInit(const Prog *item, char *buf, size_t buf_size) {
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR FLOAT_NUM ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR FLOAT_NUM ACP_DELIMITER_COLUMN_STR FLOAT_NUM ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR FLOAT_NUM ACP_DELIMITER_COLUMN_STR FLOAT_NUM ACP_DELIMITER_ROW_STR,
            item->id,
            item->reg.change_gap.tv_sec,
            item->reg.goal,
            item->reg.heater.use,
            item->reg.heater.delta,
            item->reg.heater.em.pwm_rsl,
            item->reg.cooler.use,
            item->reg.cooler.delta,
            item->reg.cooler.em.pwm_rsl
            );
    if (bufCat(buf, q, buf_size) == NULL) {
        return 0;
    }
    return 1;
}

int sendStrPack(char qnf, char *cmd) {
    extern Peer peer_client;
    return acp_sendStrPack(qnf, cmd, &peer_client);
}

int sendBufPack(char *buf, char qnf, char *cmd_str) {
    extern Peer peer_client;
    return acp_sendBufPack(buf, qnf, cmd_str, &peer_client);
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
    sendStr("+-----------------------------------------------------------------------------------------------------------------------------------+\n", &crc);
    sendStr("|                                                             Program                                                               |\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    sendStr("|    id     |    goal   |  delta_h  |  delta_c  | change_gap|change_rest|   state   |  state_r  | state_onf | out_heater| out_cooler|\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    PROG_LIST_LOOP_DF
    PROG_LIST_LOOP_ST
            char *state = reg_getStateStr(curr->reg.state);
    char *state_r = reg_getStateStr(curr->reg.state_r);
    char *state_onf = reg_getStateStr(curr->reg.state_onf);
    struct timespec tm1 = getTimeRestChange(curr);
    snprintf(q, sizeof q, "|%11d|%11.3f|%11.3f|%11.3f|%11ld|%11ld|%11s|%11s|%11s|%11.3f|%11.3f|\n",
            curr->id,
            curr->reg.goal,
            curr->reg.heater.delta,
            curr->reg.cooler.delta,
            curr->reg.change_gap.tv_sec,
            tm1.tv_sec,
            state,
            state_r,
            state_onf,
            curr->reg.heater.output,
            curr->reg.cooler.output
            );
    sendStr(q, &crc);
    PROG_LIST_LOOP_SP
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);

    sendStr("+-------------------------------------------------------------------------------------+\n", &crc);
    sendStr("|                                       Peer                                          |\n", &crc);
    sendStr("+--------------------------------+-----------+-----------+----------------+-----------+\n", &crc);
    sendStr("|               id               |   link    | sock_port |      addr      |     fd    |\n", &crc);
    sendStr("+--------------------------------+-----------+-----------+----------------+-----------+\n", &crc);
    for (i = 0; i < pl->length; i++) {
        snprintf(q, sizeof q, "|%32s|%11p|%11u|%16u|%11d|\n",
                pl->item[i].id,
                (void *)&pl->item[i],
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

            curr->reg.heater.em.id,
            curr->reg.heater.em.remote_id,
            curr->reg.heater.em.pwm_rsl,
           (void *) curr->reg.heater.em.source,

            curr->reg.cooler.em.id,
            curr->reg.cooler.em.remote_id,
            curr->reg.cooler.em.pwm_rsl,
            (void *)curr->reg.cooler.em.source
            );
    sendStr(q, &crc);
    PROG_LIST_LOOP_SP
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    sendStr("+-----------+------------------------------------------------------------------------------+\n", &crc);
    sendStr("|    Prog   |                                   Sensor                                     |\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+------------------------------------------+\n", &crc);
    sendStr("|           |           |           |           |                   value                  |\n", &crc);
    sendStr("|           |           |           |           |-----------+-----------+-----------+------+\n", &crc);
    sendStr("|    id     |    id     | remote_id | peer_link |   value   |    sec    |   nsec    | state|\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+------+\n", &crc);
    PROG_LIST_LOOP_ST
    snprintf(q, sizeof q, "|%11d|%11d|%11d|%11p|%11f|%11ld|%11ld|%6d|\n",
            curr->id,
            curr->reg.sensor.id,
            curr->reg.sensor.remote_id,
            (void *)curr->reg.sensor.source,
            curr->reg.sensor.value.value,
            curr->reg.sensor.value.tm.tv_sec,
            curr->reg.sensor.value.tm.tv_nsec,
            curr->reg.sensor.value.state
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
    snprintf(q, sizeof q, "%c\tunload program from RAM, after that load it; program id expected if '.' quantifier is used\n", ACP_CMD_RESET);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tenable running program; program id expected if '.' quantifier is used\n", ACP_CMD_REGONF_PROG_ENABLE);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tdisable running program; program id expected if '.' quantifier is used\n", ACP_CMD_REGONF_PROG_DISABLE);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tset heater power; program id and value expected\n", ACP_CMD_REGONF_PROG_SET_HEATER_POWER);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tset cooler power; program id and value expected\n", ACP_CMD_REGONF_PROG_SET_COOLER_POWER);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tset goal; program id and value expected\n", ACP_CMD_REGONF_PROG_SET_GOAL);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tset regulator EM mode (cooler or heater or both); program id and value expected\n", ACP_CMD_REGONF_PROG_SET_EM_MODE);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tset heater delta; program id and value expected\n", ACP_CMD_REGONF_PROG_SET_HEATER_DELTA);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tset cooler delta; program id and value expected\n", ACP_CMD_REGONF_PROG_SET_COOLER_DELTA);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tget prog runtime data in format:  progId\\tstate\\tstateEM\\toutputHeater\\toutputCooler\\ttimeRestSecToEMSwap\\tsensorValue\\tsensorState; program id expected if '.' quantifier is used\n", ACP_CMD_REGONF_PROG_GET_DATA_RUNTIME);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tget prog initial data in format;  progId\\tsetPoint\\tdeltaHeater\\tdeltaCooler\\tchangeGap; program id expected if '.' quantifier is used\n", ACP_CMD_REGONF_PROG_GET_DATA_INIT);
    sendStr(q, &crc);
    sendFooter(crc);
}
