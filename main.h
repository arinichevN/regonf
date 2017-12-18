
#ifndef REGONF_MAIN_H
#define REGONF_MAIN_H


#include "lib/dbl.h"
#include "lib/util.h"
#include "lib/crc.h"
#include "lib/gpio.h"
#include "lib/app.h"
#include "lib/configl.h"
#include "lib/timef.h"
#include "lib/udp.h"
#include "lib/regonfhc.h"
#include "lib/acp/main.h"
#include "lib/acp/app.h"
#include "lib/acp/regulator.h"
#include "lib/acp/prog.h"
#include "lib/acp/regonf.h"

#define APP_NAME regonf
#define APP_NAME_STR TOSTRING(APP_NAME)

#ifdef MODE_FULL
#define CONF_DIR "/etc/controller/" APP_NAME_STR "/"
#endif
#ifndef MODE_FULL
#define CONF_DIR "./"
#endif
#define CONFIG_FILE "" CONF_DIR "config.tsv"

#define PROG_FIELDS "id,sensor_id,heater_em_id,em_mode,cooler_em_id,goal,heater_delta,cooler_delta,change_gap,enable,load"

#define WAIT_RESP_TIMEOUT 3

#define MODE_PID_STR "pid"
#define MODE_ONF_STR "onf"
#define MODE_SIZE 3

#define FLOAT_NUM "%.1f"


#define PROG_LIST_LOOP_DF Prog *curr = prog_list.top;
#define PROG_LIST_LOOP_ST while (curr != NULL) {
#define PROG_LIST_LOOP_SP curr = curr->next; } curr = prog_list.top;

struct prog_st {
    int id;
    RegOnfHC reg;
    Mutex mutex;
    struct prog_st *next;
};

typedef struct prog_st Prog;

DEC_LLIST(Prog)

typedef struct {
    sqlite3 *db;
    PeerList *peer_list;
    ProgList *prog_list;
} ProgData;

extern int readSettings() ;

extern void initApp() ;

extern int initData() ;

extern void serverRun(int *state, int init_state) ;

extern void progControl(Prog *item) ;

extern void *threadFunction(void *arg) ;

extern int createThread_ctl() ;

extern void freeProg(ProgList * list) ;

extern void freeData() ;

extern void freeApp() ;

extern void exit_nicely() ;

extern void exit_nicely_e(char *s) ;

#endif 

