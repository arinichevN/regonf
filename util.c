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

int bufCatProgRuntime(const Prog *item, ACPResponse *response) {
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
    return acp_responseStrCat(response, q);
}

int bufCatProgInit(const Prog *item, ACPResponse *response) {
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
    return acp_responseStrCat(response, q);
}

void printData(ACPResponse *response) {
    ProgList *list=&prog_list; PeerList *pl=&peer_list;
    char q[LINE_SIZE];
    size_t i;
    snprintf(q, sizeof q, "CONFIG_FILE: %s\n", CONFIG_FILE);
    SEND_STR(q)
    snprintf(q, sizeof q, "port: %d\n", sock_port);
    SEND_STR(q)
    snprintf(q, sizeof q, "pid_path: %s\n", pid_path);
    SEND_STR(q)
    snprintf(q, sizeof q, "cycle_duration sec: %ld\n", cycle_duration.tv_sec);
    SEND_STR(q)
    snprintf(q, sizeof q, "cycle_duration nsec: %ld\n", cycle_duration.tv_nsec);
    SEND_STR(q)
    snprintf(q, sizeof q, "db_data_path: %s\n", db_data_path);
    SEND_STR(q)
    snprintf(q, sizeof q, "db_public_path: %s\n", db_public_path);
    SEND_STR(q)
    snprintf(q, sizeof q, "app_state: %s\n", getAppState(app_state));
    SEND_STR(q)
    snprintf(q, sizeof q, "PID: %d\n", proc_id);
    SEND_STR(q)
    snprintf(q, sizeof q, "prog_list length: %d\n", list->length);
    SEND_STR(q)
    SEND_STR("+-----------------------------------------------------------------------------------------------------------------------------------+\n")
    SEND_STR("|                                                             Program                                                               |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|    id     |    goal   |  delta_h  |  delta_c  | change_gap|change_rest|   state   |  state_r  | state_onf | out_heater| out_cooler|\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
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
    SEND_STR(q)
    PROG_LIST_LOOP_SP
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")

    SEND_STR("+-------------------------------------------------------------------------------------+\n")
    SEND_STR("|                                       Peer                                          |\n")
    SEND_STR("+--------------------------------+-----------+-----------+----------------+-----------+\n")
    SEND_STR("|               id               |   link    | sock_port |      addr      |     fd    |\n")
    SEND_STR("+--------------------------------+-----------+-----------+----------------+-----------+\n")
    for (i = 0; i < pl->length; i++) {
        snprintf(q, sizeof q, "|%32s|%11p|%11u|%16u|%11d|\n",
                pl->item[i].id,
                (void *)&pl->item[i],
                pl->item[i].addr.sin_port,
                pl->item[i].addr.sin_addr.s_addr,
                *pl->item[i].fd
                );
        SEND_STR(q)
    }
    SEND_STR("+--------------------------------+-----------+-----------+----------------+-----------+\n")

    SEND_STR("+-----------------------------------------------------------------------------------------------------------+\n")
    SEND_STR("|                                                    Prog EM                                                |\n")
    SEND_STR("+-----------+-----------------------------------------------+-----------------------------------------------+\n")
    SEND_STR("|           |                   EM heater                   |                  EM cooler                    |\n")
    SEND_STR("+           +-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|  prog_id  |     id    | remote_id |  pwm_rsl  | peer_link |     id    | remote_id |  pwm_rsl  | peer_link |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
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
    SEND_STR(q)
    PROG_LIST_LOOP_SP
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    SEND_STR("+-----------+------------------------------------------------------------------------------+\n")
    SEND_STR("|    Prog   |                                   Sensor                                     |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+------------------------------------------+\n")
    SEND_STR("|           |           |           |           |                   value                  |\n")
    SEND_STR("|           |           |           |           |-----------+-----------+-----------+------+\n")
    SEND_STR("|    id     |    id     | remote_id | peer_link |   value   |    sec    |   nsec    | state|\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+------+\n")
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
    SEND_STR(q)
    PROG_LIST_LOOP_SP
    SEND_STR_L("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+------+\n")
}

void printHelp(ACPResponse *response) {
    char q[LINE_SIZE];
    SEND_STR("COMMAND LIST\n")
    snprintf(q, sizeof q, "%s\tput process into active mode; process will read configuration\n", ACP_CMD_APP_START);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tput process into standby mode; all running programs will be stopped\n", ACP_CMD_APP_STOP);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tfirst stop and then start process\n", ACP_CMD_APP_RESET);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tterminate process\n", ACP_CMD_APP_EXIT);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget state of process; response: B - process is in active mode, I - process is in standby mode\n", ACP_CMD_APP_PING);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget some variable's values; response will be packed into multiple packets\n", ACP_CMD_APP_PRINT);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget this help; response will be packed into multiple packets\n", ACP_CMD_APP_HELP);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tload prog into RAM and start its execution; program id expected\n", ACP_CMD_PROG_START);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tunload program from RAM; program id expected\n", ACP_CMD_PROG_STOP);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tunload program from RAM, after that load it; program id expected\n", ACP_CMD_PROG_RESET);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tenable running program; program id expected\n", ACP_CMD_PROG_ENABLE);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tdisable running program; program id expected\n", ACP_CMD_PROG_DISABLE);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tset heater power; program id and value expected\n", ACP_CMD_REG_PROG_SET_HEATER_POWER);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tset cooler power; program id and value expected\n", ACP_CMD_REG_PROG_SET_COOLER_POWER);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tset goal; program id and value expected\n", ACP_CMD_REG_PROG_SET_GOAL);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tset regulator EM mode (cooler or heater or both); program id and value expected\n", ACP_CMD_REG_PROG_SET_EM_MODE);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tset heater delta; program id and value expected\n", ACP_CMD_REGONF_PROG_SET_HEATER_DELTA);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tset cooler delta; program id and value expected\n", ACP_CMD_REGONF_PROG_SET_COOLER_DELTA);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget prog runtime data in format:  progId\\tstate\\tstateEM\\toutputHeater\\toutputCooler\\ttimeRestSecToEMSwap\\tsensorValue\\tsensorState; program id expected\n", ACP_CMD_PROG_GET_DATA_RUNTIME);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget prog initial data in format;  progId\\tsetPoint\\tdeltaHeater\\tdeltaCooler\\tchangeGap; program id expected\n", ACP_CMD_PROG_GET_DATA_INIT);
    SEND_STR_L(q)
}
