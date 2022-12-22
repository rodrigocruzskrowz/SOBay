#ifndef SOBay_BACKEND_H
#define SOBay_BACKEND_H
#include "utils.h"
#include "users_lib.h"
#include "utils.h"
#include "struct.h"
#include <pthread.h>

char *FUSERS;
char *FITEMS;
char *FPROMOTERS;
char *FINIT = "init.txt";

int TEMPO;
int PROX_ID;

typedef struct ThreadLogData LogData;
struct ThreadLogData{
    User ut;
    User *connUt;
    int *nConnUt;
    int fd_sv_fifo;
    pthread_mutex_t *ptrinco;
    int para;
};

typedef struct ThreadTempoData Tempo;
struct ThreadTempoData{
    Item it;
    int para;
};

typedef struct ThreadRequestRespondeData RR;
struct ThreadRequestRespondeData{
    CA ca;
    Item *listIt;
    int *nlistIt;
    int fd_sv_fifo;
    int para;
};

#define NUM_OF_THREADS 1

void initPlataforma();
void *validaLogin(void *logdata);
void addUserConnection(User ut, User *connUt, int *nusers);

#endif //SOBay_BACKEND_H
