#ifndef SOBay_FRONTEND_H
#define SOBay_FRONTEND_H
#include <pthread.h>

typedef struct Utilizador User;
struct Utilizador{
    char nome[100];
    char password[100];
    int saldo;
    int valid;
    int pid;
    pthread_mutex_t *u;
};

#endif //SOBay_FRONTEND_H
