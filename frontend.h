#ifndef SOBay_FRONTEND_H
#define SOBay_FRONTEND_H
#include "utils.h"

typedef struct ThreadHeartbeatData HB;
struct ThreadHeartbeatData{
    int pid;
    char nomeUser[MAX_SIZE];
    int heartbeat;
    int fd_hb_fifo;
    pthread_mutex_t *ptrinco;
    int para;
};

void *sendHeartBeat(void *pdata);
void encerraFrontend(int sign);

#endif //SOBay_FRONTEND_H
