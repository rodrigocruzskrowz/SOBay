#ifndef SOBay_BACKEND_H
#define SOBay_BACKEND_H
#include "utils.h"
#include "users_lib.h"
#include "utils.h"
#include "struct.h"

char *FUSERS;
char *FITEMS;
char *FPROMOTERS;
char *FINIT = "init.txt";

int TEMPO;
int PROX_ID;

typedef struct ThreadData TData;
struct ThreadData{

    pthread_mutex_t *ptrinco;
};

void initPlataforma();

#endif //SOBay_BACKEND_H
