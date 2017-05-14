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
size_t sock_buf_size = 0;
int sock_fd = -1;
int sock_fd_tf = -1;
Peer peer_client = {.fd = &sock_fd, .addr_size = sizeof peer_client.addr};
struct timespec cycle_duration = {0, 0};
pthread_t thread;
char thread_cmd;
Mutex progl_mutex = {.created = 0, .attr_initialized = 0};
I1List i1l = {NULL, 0};
I2List i2l = {NULL, 0};
S1List s1l = {NULL, 0};
I1S1List i1s1l = {NULL, 0};
I1F1List i1f1l = {NULL, 0};
F1List f1l = {NULL, 0};
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
        fputs("ERROR: readSettings: fopen\n", stderr);
#endif
        return 0;
    }

    int n;
    n = fscanf(stream, "%d\t%255s\t%d\t%ld\t%ld\t%255s\t%255s\n",
            &sock_port,
            pid_path,
            &sock_buf_size,
            &cycle_duration.tv_sec,
            &cycle_duration.tv_nsec,
            db_data_path,
            db_public_path
            );
    if (n != 7) {
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
    peer_client.sock_buf_size = sock_buf_size;
    if (!initPid(&pid_file, &proc_id, pid_path)) {
        exit_nicely_e("initApp: failed to initialize pid\n");
    }
#ifdef MODE_DEBUG
    printf("initApp: CONFIG_FILE: %s\n", CONFIG_FILE);
    printf("initApp: PID: %d\n", proc_id);
    printf("initApp: sock_port: %d\n", sock_port);
    printf("initApp: sock_buf_size: %d\n", sock_buf_size);
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
    if (!config_getPeerList(&peer_list, &sock_fd_tf, sock_buf_size, db_public_path)) {
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
    i1l.item = (int *) malloc(sock_buf_size * sizeof *(i1l.item));
    if (i1l.item == NULL) {
#ifdef MODE_DEBUG
        fputs("initData: ERROR: failed to allocate memory for i1l\n", stderr);
#endif
        freeProg(&prog_list);
        FREE_LIST(&peer_list);
        return 0;
    }
    i1f1l.item = (I1F1 *) malloc(sock_buf_size * sizeof *(i1f1l.item));
    if (i1f1l.item == NULL) {
#ifdef MODE_DEBUG
        fputs("initData: ERROR: failed to allocate memory for i1f1l\n", stderr);
#endif
        FREE_LIST(&i1l);
        freeProg(&prog_list);
        FREE_LIST(&peer_list);
        return 0;
    }
    f1l.item = (float *) malloc(sock_buf_size * sizeof *(f1l.item));
    if (f1l.item == NULL) {
#ifdef MODE_DEBUG
        fputs("initData: ERROR: failed to allocate memory for f1l\n", stderr);
#endif
        FREE_LIST(&i1f1l);
        FREE_LIST(&i1l);
        freeProg(&prog_list);
        FREE_LIST(&peer_list);
        return 0;
    }
    i2l.item = (I2 *) malloc(sock_buf_size * sizeof *(i2l.item));
    if (i2l.item == NULL) {
#ifdef MODE_DEBUG
        fputs("initData: ERROR: failed to allocate memory for i2l\n", stderr);
#endif
        FREE_LIST(&f1l);
        FREE_LIST(&i1f1l);
        FREE_LIST(&i1l);
        freeProg(&prog_list);
        FREE_LIST(&peer_list);
        return 0;
    }
    s1l.item = (char *) malloc(sock_buf_size * sizeof *(s1l.item));
    if (s1l.item == NULL) {
#ifdef MODE_DEBUG
        fputs("initData: ERROR: failed to allocate memory for s1l\n", stderr);
#endif
        FREE_LIST(&i2l);
        FREE_LIST(&f1l);
        FREE_LIST(&i1f1l);
        FREE_LIST(&i1l);
        freeProg(&prog_list);
        FREE_LIST(&peer_list);
        return 0;
    }
    i1s1l.item = (I1S1 *) malloc(sock_buf_size * sizeof *(i1s1l.item));
    if (i1s1l.item == NULL) {
#ifdef MODE_DEBUG
        fputs("initData: ERROR: failed to allocate memory for i1s1l\n", stderr);
#endif
        FREE_LIST(&s1l);
        FREE_LIST(&i2l);
        FREE_LIST(&f1l);
        FREE_LIST(&i1f1l);
        FREE_LIST(&i1l);
        freeProg(&prog_list);
        FREE_LIST(&peer_list);
        return 0;
    }
    if (!createThread_ctl()) {
#ifdef MODE_DEBUG
        fputs("initData: ERROR: failed to create thread\n", stderr);
#endif
        FREE_LIST(&i1s1l);
        FREE_LIST(&s1l);
        FREE_LIST(&i2l);
        FREE_LIST(&f1l);
        FREE_LIST(&i1f1l);
        FREE_LIST(&i1l);
        freeProg(&prog_list);
        FREE_LIST(&peer_list);
        return 0;
    }
    return 1;
}

void serverRun(int *state, int init_state) {
    char buf_in[sock_buf_size];
    char buf_out[sock_buf_size];
    uint8_t crc;
    crc = 0;
    memset(buf_in, 0, sizeof buf_in);
    acp_initBuf(buf_out, sizeof buf_out);
    if (recvfrom(sock_fd, buf_in, sizeof buf_in, 0, (struct sockaddr*) (&(peer_client.addr)), &(peer_client.addr_size)) < 0) {
#ifdef MODE_DEBUG
        perror("serverRun: recvfrom() error");
#endif
    }
#ifdef MODE_DEBUG
    acp_dumpBuf(buf_in, sizeof buf_in);
#endif    
    if (!acp_crc_check(buf_in, sizeof buf_in)) {
#ifdef MODE_DEBUG
        fputs("WARNING: serverRun: crc check failed\n", stderr);
#endif
        return;
    }
    switch (buf_in[1]) {
        case ACP_CMD_APP_START:
            if (!init_state) {
                *state = APP_INIT_DATA;
            }
            return;
        case ACP_CMD_APP_STOP:
            if (init_state) {
                *state = APP_STOP;
            }
            return;
        case ACP_CMD_APP_RESET:
            *state = APP_RESET;
            return;
        case ACP_CMD_APP_EXIT:
            *state = APP_EXIT;
            return;
        case ACP_CMD_APP_PING:
            if (init_state) {
                sendStrPack(ACP_QUANTIFIER_BROADCAST, ACP_RESP_APP_BUSY);
            } else {
                sendStrPack(ACP_QUANTIFIER_BROADCAST, ACP_RESP_APP_IDLE);
            }
            return;
        case ACP_CMD_APP_PRINT:
            printAll(&prog_list, &peer_list);
            return;
        case ACP_CMD_APP_HELP:
            printHelp();
            return;
        default:
            if (!init_state) {
                return;
            }
            break;
    }

    switch (buf_in[0]) {
        case ACP_QUANTIFIER_BROADCAST:
        case ACP_QUANTIFIER_SPECIFIC:
            break;
        default:
            return;
    }

    switch (buf_in[1]) {
        case ACP_CMD_STOP:
        case ACP_CMD_START:
        case ACP_CMD_RESET:
        case ACP_CMD_REGONF_PROG_ENABLE:
        case ACP_CMD_REGONF_PROG_DISABLE:
        case ACP_CMD_REGONF_PROG_GET_DATA_RUNTIME:
        case ACP_CMD_REGONF_PROG_GET_DATA_INIT:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                    break;
                case ACP_QUANTIFIER_SPECIFIC:
                    acp_parsePackI1(buf_in, &i1l, sock_buf_size);
                    if (i1l.length <= 0) {
                        return;
                    }
                    break;
            }
            break;
        case ACP_CMD_REGONF_PROG_SET_HEATER_POWER:
        case ACP_CMD_REGONF_PROG_SET_COOLER_POWER:
        case ACP_CMD_REGONF_PROG_SET_GOAL:
        case ACP_CMD_REGONF_PROG_SET_HEATER_DELTA:
        case ACP_CMD_REGONF_PROG_SET_COOLER_DELTA:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                    acp_parsePackF1(buf_in, &f1l, sock_buf_size);
                    if (f1l.length <= 0) {
                        return;
                    }
                    break;
                case ACP_QUANTIFIER_SPECIFIC:
                    acp_parsePackI1F1(buf_in, &i1f1l, sock_buf_size);
                    if (i1f1l.length <= 0) {
                        return;
                    }
                    break;
            }
            break;
        case ACP_CMD_REGONF_PROG_SET_CHANGE_GAP:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                    acp_parsePackI1(buf_in, &i1l, sock_buf_size);
                    if (i1l.length <= 0) {
                        return;
                    }
                    break;
                case ACP_QUANTIFIER_SPECIFIC:
                    acp_parsePackI2(buf_in, &i2l, sock_buf_size);
                    if (i2l.length <= 0) {
                        return;
                    }
                    break;
            }
            break;
        case ACP_CMD_REGONF_PROG_SET_EM_MODE:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                    acp_parsePackS1(buf_in, &s1l, sock_buf_size);
                    if (s1l.length <= 0) {
                        return;
                    }
                    break;
                case ACP_QUANTIFIER_SPECIFIC:
                    acp_parsePackI1S1(buf_in, &i1s1l, sock_buf_size);
                    if (i1s1l.length <= 0) {
                        return;
                    }
                    break;
            }
            break;
        default:
            return;

    }
    int i, j;
    switch (buf_in[1]) {
        case ACP_CMD_STOP:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                {
                    PROG_LIST_LOOP_DF
                    PROG_LIST_LOOP_ST
                    regonfhc_turnOff(&curr->reg);
                    deleteProgById(curr->id, &prog_list, db_data_path);
                    PROG_LIST_LOOP_SP
                    break;
                }
                case ACP_QUANTIFIER_SPECIFIC:
                    for (i = 0; i < i1l.length; i++) {
                        Prog *curr = getProgById(i1l.item[i], &prog_list);
                        if (curr != NULL) {
                            regonfhc_turnOff(&curr->reg);
                            deleteProgById(i1l.item[i], &prog_list, db_data_path);
                        }
                    }
                    break;
            }
            return;
        case ACP_CMD_START:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                {
                    loadAllProg(&prog_list, &peer_list, db_data_path);
                    break;
                }
                case ACP_QUANTIFIER_SPECIFIC:

                    for (i = 0; i < i1l.length; i++) {
                        addProgById(i1l.item[i], &prog_list, &peer_list, db_data_path);
                    }

                    break;
            }
            return;
        case ACP_CMD_RESET:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                {
                    PROG_LIST_LOOP_DF
                    PROG_LIST_LOOP_ST
                    regonfhc_turnOff(&curr->reg);
                    deleteProgById(curr->id, &prog_list, db_data_path);
                    PROG_LIST_LOOP_SP
                    loadAllProg(&prog_list, &peer_list, db_data_path);

                    break;
                }
                case ACP_QUANTIFIER_SPECIFIC:

                    for (i = 0; i < i1l.length; i++) {
                        Prog *curr = getProgById(i1l.item[i], &prog_list);
                        if (curr != NULL) {
                            regonfhc_turnOff(&curr->reg);
                            deleteProgById(i1l.item[i], &prog_list, db_data_path);
                        }
                    }
                    for (i = 0; i < i1l.length; i++) {
                        addProgById(i1l.item[i], &prog_list, &peer_list, db_data_path);
                    }

                    break;
            }
            return;
        case ACP_CMD_REGONF_PROG_ENABLE:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                {

                    PROG_LIST_LOOP_DF
                    PROG_LIST_LOOP_ST
                    if (lockProg(curr)) {
                        regonfhc_enable(&curr->reg);
                        saveProgEnable(curr->id, 1, db_data_path);
                        unlockProg(curr);
                    }
                    PROG_LIST_LOOP_SP
                    break;
                }
                case ACP_QUANTIFIER_SPECIFIC:
                    for (i = 0; i < i1l.length; i++) {
                        Prog *curr = getProgById(i1l.item[i], &prog_list);
                        if (curr != NULL) {
                            if (lockProg(curr)) {
                                regonfhc_enable(&curr->reg);
                                saveProgEnable(curr->id, 1, db_data_path);
                                unlockProg(curr);
                            }
                        }
                    }
                    break;
            }
            return;
        case ACP_CMD_REGONF_PROG_DISABLE:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                {
                    PROG_LIST_LOOP_DF
                    PROG_LIST_LOOP_ST
                    if (lockProg(curr)) {
                        regonfhc_disable(&curr->reg);
                        saveProgEnable(curr->id, 0, db_data_path);
                        unlockProg(curr);
                    }
                    PROG_LIST_LOOP_SP
                    break;
                }
                case ACP_QUANTIFIER_SPECIFIC:
                    for (i = 0; i < i1l.length; i++) {
                        Prog *curr = getProgById(i1l.item[i], &prog_list);
                        if (curr != NULL) {
                            if (lockProg(curr)) {
                                regonfhc_disable(&curr->reg);
                                saveProgEnable(curr->id, 0, db_data_path);
                                unlockProg(curr);
                            }
                        }
                    }
                    break;
            }
            return;
        case ACP_CMD_REGONF_PROG_GET_DATA_RUNTIME:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                {
                    PROG_LIST_LOOP_DF
                    PROG_LIST_LOOP_ST
                    if (!bufCatProgRuntime(curr, buf_out, sock_buf_size)) {
                        sendStrPack(ACP_QUANTIFIER_BROADCAST, ACP_RESP_BUF_OVERFLOW);
                        return;
                    }
                    PROG_LIST_LOOP_SP
                    break;
                }
                case ACP_QUANTIFIER_SPECIFIC:
                    for (i = 0; i < i1l.length; i++) {
                        Prog *curr = getProgById(i1l.item[i], &prog_list);
                        if (curr != NULL) {
                            if (!bufCatProgRuntime(curr, buf_out, sock_buf_size)) {
                                sendStrPack(ACP_QUANTIFIER_BROADCAST, ACP_RESP_BUF_OVERFLOW);
                                return;
                            }
                        }
                    }
                    break;
            }
            break;
        case ACP_CMD_REGONF_PROG_GET_DATA_INIT:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                {
                    PROG_LIST_LOOP_DF
                    PROG_LIST_LOOP_ST
                    if (!bufCatProgInit(curr, buf_out, sock_buf_size)) {
                        sendStrPack(ACP_QUANTIFIER_BROADCAST, ACP_RESP_BUF_OVERFLOW);
                        return;
                    }
                    PROG_LIST_LOOP_SP
                    break;
                }
                case ACP_QUANTIFIER_SPECIFIC:
                    for (i = 0; i < i1l.length; i++) {
                        Prog *curr = getProgById(i1l.item[i], &prog_list);
                        if (curr != NULL) {
                            if (!bufCatProgInit(curr, buf_out, sock_buf_size)) {
                                sendStrPack(ACP_QUANTIFIER_BROADCAST, ACP_RESP_BUF_OVERFLOW);
                                return;
                            }
                        }
                    }
                    break;
            }
            break;
        case ACP_CMD_REGONF_PROG_SET_HEATER_POWER:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                {
                    PROG_LIST_LOOP_DF
                    PROG_LIST_LOOP_ST
                    if (lockProg(curr)) {
                        regonfhc_setHeaterPower(&curr->reg, f1l.item[0]);
                        unlockProg(curr);
                    }
                    PROG_LIST_LOOP_SP
                    break;
                }
                case ACP_QUANTIFIER_SPECIFIC:
                    for (i = 0; i < i1l.length; i++) {
                        Prog *curr = getProgById(i1f1l.item[i].p0, &prog_list);
                        if (curr != NULL) {
                            if (lockProg(curr)) {
                                regonfhc_setHeaterPower(&curr->reg, i1f1l.item[i].p1);
                                unlockProg(curr);
                            }
                        }
                    }
                    break;
            }
            return;
        case ACP_CMD_REGONF_PROG_SET_COOLER_POWER:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                {
                    PROG_LIST_LOOP_DF
                    PROG_LIST_LOOP_ST
                    if (lockProg(curr)) {
                        regonfhc_setCoolerPower(&curr->reg, f1l.item[0]);
                        unlockProg(curr);
                    }
                    PROG_LIST_LOOP_SP
                    break;
                }
                case ACP_QUANTIFIER_SPECIFIC:
                    for (i = 0; i < i1l.length; i++) {
                        Prog *curr = getProgById(i1f1l.item[i].p0, &prog_list);
                        if (curr != NULL) {
                            if (lockProg(curr)) {
                                regonfhc_setCoolerPower(&curr->reg, i1f1l.item[i].p1);
                                unlockProg(curr);
                            }
                        }
                    }
                    break;
            }
            return;
        case ACP_CMD_REGONF_PROG_SET_GOAL:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                {
                    PROG_LIST_LOOP_DF
                    PROG_LIST_LOOP_ST
                    if (lockProg(curr)) {
                        regonfhc_setGoal(&curr->reg, f1l.item[0]);
                        unlockProg(curr);
                    }
                    saveProgFieldFloat(curr->id, f1l.item[0], db_data_path, "goal");
                    PROG_LIST_LOOP_SP
                    break;
                }
                case ACP_QUANTIFIER_SPECIFIC:
                    for (i = 0; i < i1f1l.length; i++) {
                        Prog *curr = getProgById(i1f1l.item[i].p0, &prog_list);
                        if (curr != NULL) {
                            if (lockProg(curr)) {
                                regonfhc_setGoal(&curr->reg, i1f1l.item[i].p1);
                                unlockProg(curr);
                            }
                        }
                        saveProgFieldFloat(i1f1l.item[i].p0, i1f1l.item[i].p1, db_data_path, "goal");
                    }
                    break;
            }
            return;
        case ACP_CMD_REGONF_PROG_SET_HEATER_DELTA:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                {
                    PROG_LIST_LOOP_DF
                    PROG_LIST_LOOP_ST
                    if (lockProg(curr)) {
                        regonfhc_setHeaterDelta(&curr->reg, f1l.item[0]);
                        unlockProg(curr);
                    }
                    saveProgFieldFloat(curr->id, f1l.item[0], db_data_path, "heater_delta");
                    PROG_LIST_LOOP_SP
                    break;
                }
                case ACP_QUANTIFIER_SPECIFIC:
                    for (i = 0; i < i1f1l.length; i++) {
                        Prog *curr = getProgById(i1f1l.item[i].p0, &prog_list);
                        if (curr != NULL) {
                            if (lockProg(curr)) {
                                regonfhc_setHeaterDelta(&curr->reg, i1f1l.item[i].p1);
                                unlockProg(curr);
                            }
                        }
                        saveProgFieldFloat(i1f1l.item[i].p0, i1f1l.item[i].p1, db_data_path, "heater_delta");
                    }
                    break;
            }
            return;
        case ACP_CMD_REGONF_PROG_SET_COOLER_DELTA:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                {
                    PROG_LIST_LOOP_DF
                    PROG_LIST_LOOP_ST
                    if (lockProg(curr)) {
                        regonfhc_setCoolerDelta(&curr->reg, f1l.item[0]);
                        unlockProg(curr);
                    }
                    saveProgFieldFloat(i1f1l.item[i].p0, i1f1l.item[i].p1, db_data_path, "cooler_delta");
                    PROG_LIST_LOOP_SP
                    break;
                }
                case ACP_QUANTIFIER_SPECIFIC:
                    for (i = 0; i < i1f1l.length; i++) {
                        Prog *curr = getProgById(i1f1l.item[i].p0, &prog_list);
                        if (curr != NULL) {
                            if (lockProg(curr)) {
                                regonfhc_setCoolerDelta(&curr->reg, i1f1l.item[i].p1);
                                unlockProg(curr);
                            }
                        }
                        saveProgFieldFloat(i1f1l.item[i].p0, i1f1l.item[i].p1, db_data_path, "cooler_delta");
                    }
                    break;
            }
            return;
        case ACP_CMD_REGONF_PROG_SET_CHANGE_GAP:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                {
                    PROG_LIST_LOOP_DF
                    PROG_LIST_LOOP_ST
                    if (lockProg(curr)) {
                        regonfhc_setChangeGap(&curr->reg, i1l.item[0]);
                        unlockProg(curr);
                    }
                    saveProgFieldInt(curr->id, i1l.item[0], db_data_path, "change_gap");
                    PROG_LIST_LOOP_SP
                    break;
                }
                case ACP_QUANTIFIER_SPECIFIC:
                    for (i = 0; i < i2l.length; i++) {
                        Prog *curr = getProgById(i2l.item[i].p0, &prog_list);
                        if (curr != NULL) {
                            if (lockProg(curr)) {
                                regonfhc_setChangeGap(&curr->reg, i2l.item[i].p1);
                                unlockProg(curr);
                            }
                        }
                        saveProgFieldInt(i2l.item[i].p0, i2l.item[i].p1, db_data_path, "change_gap");
                    }
                    break;
            }
            return;
        case ACP_CMD_REGONF_PROG_SET_EM_MODE:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                {
                    PROG_LIST_LOOP_DF
                    PROG_LIST_LOOP_ST
                    if (lockProg(curr)) {
                        regonfhc_setEMMode(&curr->reg, &s1l.item[0]);
                        unlockProg(curr);
                    }
                    saveProgFieldText(curr->id, &s1l.item[0], db_data_path, "em_mode");
                    PROG_LIST_LOOP_SP
                    break;
                }
                case ACP_QUANTIFIER_SPECIFIC:
                    for (i = 0; i < i1s1l.length; i++) {
                        Prog *curr = getProgById(i1s1l.item[i].p0, &prog_list);
                        if (curr != NULL) {
                            if (lockProg(curr)) {
                                regonfhc_setEMMode(&curr->reg, i1s1l.item[i].p1);
                                unlockProg(curr);
                            }
                        }
                        saveProgFieldText(i1s1l.item[i].p0, i1s1l.item[i].p1, db_data_path, "em_mode");
                    }
                    break;
            }
            return;

    }

    switch (buf_in[1]) {
        case ACP_CMD_REGONF_PROG_GET_DATA_RUNTIME:
        case ACP_CMD_REGONF_PROG_GET_DATA_INIT:
            if (!sendBufPack(buf_out, ACP_QUANTIFIER_SPECIFIC, ACP_RESP_REQUEST_SUCCEEDED)) {
                sendStrPack(ACP_QUANTIFIER_BROADCAST, ACP_RESP_BUF_OVERFLOW);
                return;
            }
            return;

    }

}

void progControl(Prog *item) {
#ifdef MODE_DEBUG
    printf("progId: %d ", item->id);
#endif
    regonfhc_onf(&item->reg);
}

void *threadFunction(void *arg) {
    char *cmd = (char *) arg;
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


            switch (*cmd) {
                case ACP_CMD_APP_STOP:
                case ACP_CMD_APP_RESET:
                case ACP_CMD_APP_EXIT:
                    *cmd = ACP_CMD_APP_NO;
                    return (EXIT_SUCCESS);
                default:
                    break;
            }
        }
        switch (*cmd) {
            case ACP_CMD_APP_STOP:
            case ACP_CMD_APP_RESET:
            case ACP_CMD_APP_EXIT:
                *cmd = ACP_CMD_APP_NO;
                return (EXIT_SUCCESS);
            default:
                break;
        }
        sleepRest(cycle_duration, t1);
    }
}

int createThread_ctl() {
    if (pthread_create(&thread, NULL, &threadFunction, (void *) &thread_cmd) != 0) {
        perror("createThread_ctl: pthread_create");
        return 0;
    }
    return 1;
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
    waitThread_ctl(ACP_CMD_APP_EXIT);
    secure();
    freeProg(&prog_list);
    FREE_LIST(&i1s1l);
    FREE_LIST(&s1l);
    FREE_LIST(&i2l);
    FREE_LIST(&f1l);
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