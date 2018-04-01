#include "main.h"

FUN_LLIST_GET_BY_ID(Prog)

extern int getProgByIdFDB(int prog_id, Prog *item, EMList *em_list, SensorFTSList *sensor_list, sqlite3 *dbl, const char *db_path);

void stopProgThread(Prog *item) {
#ifdef MODE_DEBUG
    printf("signaling thread %d to cancel...\n", item->id);
#endif
    if (pthread_cancel(item->thread) != 0) {
#ifdef MODE_DEBUG
        perror("pthread_cancel()");
#endif
    }
    void * result;
#ifdef MODE_DEBUG
    printf("joining thread %d...\n", item->id);
#endif
    if (pthread_join(item->thread, &result) != 0) {
#ifdef MODE_DEBUG
        perror("pthread_join()");
#endif
    }
    if (result != PTHREAD_CANCELED) {
#ifdef MODE_DEBUG
        printf("thread %d not canceled\n", item->id);
#endif
    }
}

void stopAllProgThreads(ProgList * list) {
    PROG_LIST_LOOP_ST
#ifdef MODE_DEBUG
            printf("signaling thread %d to cancel...\n", item->id);
#endif
    if (pthread_cancel(item->thread) != 0) {
#ifdef MODE_DEBUG
        perror("pthread_cancel()");
#endif
    }
    PROG_LIST_LOOP_SP

    PROG_LIST_LOOP_ST
            void * result;
#ifdef MODE_DEBUG
    printf("joining thread %d...\n", item->id);
#endif
    if (pthread_join(item->thread, &result) != 0) {
#ifdef MODE_DEBUG
        perror("pthread_join()");
#endif
    }
    if (result != PTHREAD_CANCELED) {
#ifdef MODE_DEBUG
        printf("thread %d not canceled\n", item->id);
#endif
    }
    PROG_LIST_LOOP_SP
}

void freeProg(Prog*item) {
    freeSocketFd(&item->sock_fd);
    freeMutex(&item->mutex);
    free(item);
}

void freeProgList(ProgList *list) {
    Prog *item = list->top, *temp;
    while (item != NULL) {
        temp = item;
        item = item->next;
        freeProg(temp);
    }
    list->top = NULL;
    list->last = NULL;
    list->length = 0;
}

int checkProg(const Prog *item) {
    return 1;
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

void secure() {
    PROG_LIST_LOOP_ST
    regonfhc_turnOff(&item->reg);
    PROG_LIST_LOOP_SP
}

struct timespec getTimeRestChange(const Prog *item) {
    return getTimeRestTmr(item->reg.change_gap, item->reg.tmr);
}

int bufCatProgRuntime(Prog *item, ACPResponse *response) {
    if (lockMutex(&item->mutex)) {
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
        unlockMutex(&item->mutex);
        return acp_responseStrCat(response, q);
    }
    return 0;
}

int bufCatProgInit(Prog *item, ACPResponse *response) {
    if (lockMutex(&item->mutex)) {
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
        unlockMutex(&item->mutex);
        return acp_responseStrCat(response, q);
    }
    return 0;
}

int bufCatProgError(Prog *item, ACPResponse *response) {
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%u" ACP_DELIMITER_ROW_STR, item->id, item->error_code);
    return acp_responseStrCat(response, q);
}

int bufCatProgGoal(Prog *item, ACPResponse *response) {
    if (lockMutex(&item->mutex)) {
        char q[LINE_SIZE];
        snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR FLOAT_NUM ACP_DELIMITER_ROW_STR,
                item->id,
                item->reg.goal
                );
        unlockMutex(&item->mutex);
        return acp_responseStrCat(response, q);
    }
    return 0;
}

int bufCatProgFTS(Prog *item, ACPResponse *response) {
    if (lockMutex(&item->mutex)) {
        int r = acp_responseFTSCat(item->id, item->reg.sensor.value.value, item->reg.sensor.value.tm, item->reg.sensor.value.state, response);
        unlockMutex(&item->mutex);
        return r;
    }
    return 0;
}

int bufCatProgEnabled(Prog *item, ACPResponse *response) {
    if (lockMutex(&item->mutex)) {
        char q[LINE_SIZE];
        int enabled = regonfhc_getEnabled(&item->reg);
        snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_ROW_STR,
                item->id,
                enabled
                );
        unlockMutex(&item->mutex);
        return acp_responseStrCat(response, q);
    }
    return 0;
}

void printData(ACPResponse *response) {
    ProgList *list = &prog_list;
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "CONFIG_FILE: %s\n", CONFIG_FILE);
    SEND_STR(q)
    snprintf(q, sizeof q, "port: %d\n", sock_port);
    SEND_STR(q)
    snprintf(q, sizeof q, "cycle_duration sec: %ld\n", cycle_duration.tv_sec);
    SEND_STR(q)
    snprintf(q, sizeof q, "cycle_duration nsec: %ld\n", cycle_duration.tv_nsec);
    SEND_STR(q)
    snprintf(q, sizeof q, "db_data_path: %s\n", db_data_path);
    SEND_STR(q)
    snprintf(q, sizeof q, "app_state: %s\n", getAppState(app_state));
    SEND_STR(q)
    snprintf(q, sizeof q, "PID: %d\n", getpid());
    SEND_STR(q)
    snprintf(q, sizeof q, "prog_list length: %d\n", list->length);
    SEND_STR(q)
    SEND_STR("+-----------------------------------------------------------------------------------------------------------------------------------------------+\n")
    SEND_STR("|                                                                   Program                                                                     |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|    id     |    goal   |  delta_h  |  delta_c  | change_gap|change_rest|   state   |  state_r  | state_onf | out_heater| out_cooler| error_code|\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    PROG_LIST_LOOP_ST
            char *state = reg_getStateStr(item->reg.state);
    char *state_r = reg_getStateStr(item->reg.state_r);
    char *state_onf = reg_getStateStr(item->reg.state_onf);
    struct timespec tm1 = getTimeRestChange(item);
    snprintf(q, sizeof q, "|%11d|%11.3f|%11.3f|%11.3f|%11ld|%11ld|%11s|%11s|%11s|%11.3f|%11.3f|%11u|\n",
            item->id,
            item->reg.goal,
            item->reg.heater.delta,
            item->reg.cooler.delta,
            item->reg.change_gap.tv_sec,
            tm1.tv_sec,
            state,
            state_r,
            state_onf,
            item->reg.heater.output,
            item->reg.cooler.output,
            item->error_code
            );
    SEND_STR(q)
    PROG_LIST_LOOP_SP
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")

    SEND_STR("+-----------------------------------------------------------------------------------+\n")
    SEND_STR("|                               Prog secure                                         |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|  prog_id  | secure_id | timeout_s |heater_out |cooler_out |  active   |   done    |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    PROG_LIST_LOOP_ST
    snprintf(q, sizeof q, "|%11d|%11d|%11ld|%11.3f|%11.3f|%11d|%11d|\n",
            item->id,
            item->reg.secure_out.id,
            item->reg.secure_out.timeout.tv_sec,
            item->reg.secure_out.heater_duty_cycle,
            item->reg.secure_out.cooler_duty_cycle,
            item->reg.secure_out.active,
            item->reg.secure_out.done
            );
    SEND_STR(q)
    PROG_LIST_LOOP_SP
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")

    SEND_STR("+-----------------------------------------------+\n")
    SEND_STR("|              Prog green light                 |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|  prog_id  |   active  |   value   |green_value|\n")
    SEND_STR("+-----------+-----------+-----------+-----------+\n")
    PROG_LIST_LOOP_ST
    snprintf(q, sizeof q, "|%11d|%11d|%11.3f|%11.3f|\n",
            item->id,
            item->reg.green_light.active,
            item->reg.green_light.sensor.value.value,
            item->reg.green_light.green_value
            );
    SEND_STR(q)
    PROG_LIST_LOOP_SP
    SEND_STR("+-----------+-----------+-----------+-----------+\n")

    acp_sendPeerListInfo(&peer_list, response, &peer_client);

    SEND_STR("+-----------------------------------------------------------------------------------------------------------+\n")
    SEND_STR("|                                                    Prog EM                                                |\n")
    SEND_STR("+-----------+-----------------------------------------------+-----------------------------------------------+\n")
    SEND_STR("|           |                   EM heater                   |                  EM cooler                    |\n")
    SEND_STR("+           +-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|  prog_id  |     id    | remote_id |  pwm_rsl  |  peer_id  |     id    | remote_id |  pwm_rsl  |  peer_id  |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    PROG_LIST_LOOP_ST
    snprintf(q, sizeof q, "|%11d|%11d|%11d|%11.3f|%11s|%11d|%11d|%11.3f|%11s|\n",
            item->id,

            item->reg.heater.em.id,
            item->reg.heater.em.remote_id,
            item->reg.heater.em.pwm_rsl,
            item->reg.heater.em.peer.id,

            item->reg.cooler.em.id,
            item->reg.cooler.em.remote_id,
            item->reg.cooler.em.pwm_rsl,
            item->reg.cooler.em.peer.id
            );
    SEND_STR(q)
    PROG_LIST_LOOP_SP
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    SEND_STR("+-----------+------------------------------------------------------------------------------+\n")
    SEND_STR("|    Prog   |                                   Sensor                                     |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+------------------------------------------+\n")
    SEND_STR("|           |           |           |           |                   value                  |\n")
    SEND_STR("|           |           |           |           |-----------+-----------+-----------+------+\n")
    SEND_STR("|    id     |    id     | remote_id |  peer_id  |   value   |    sec    |   nsec    | state|\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+------+\n")
    PROG_LIST_LOOP_ST
    snprintf(q, sizeof q, "|%11d|%11d|%11d|%11s|%11.3f|%11ld|%11ld|%6d|\n",
            item->id,
            item->reg.sensor.id,
            item->reg.sensor.remote_id,
            item->reg.sensor.peer.id,
            item->reg.sensor.value.value,
            item->reg.sensor.value.tm.tv_sec,
            item->reg.sensor.value.tm.tv_nsec,
            item->reg.sensor.value.state
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
    snprintf(q, sizeof q, "%s\tsave data to database or not; program id and value (1 || 0) expected\n", ACP_CMD_PROG_SET_SAVE);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget prog state (1-enabled, 0-disabled); program id expected\n", ACP_CMD_PROG_GET_ENABLED);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget prog goal; program id expected\n", ACP_CMD_REG_PROG_GET_GOAL);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget prog error code; program id expected\n", ACP_CMD_PROG_GET_ERROR);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget prog sensor value; program id expected\n", ACP_CMD_GET_FTS);
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
