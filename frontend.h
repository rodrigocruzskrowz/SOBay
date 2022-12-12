#ifndef SOBay_FRONTEND_H
#define SOBay_FRONTEND_H

#include "utils.h"

typedef struct Utilizador User;
struct Utilizador{
    char nome[MAX_SIZE];
    char password[MAX_SIZE];
    int saldo;
    int valid;
    int pid;
};

#endif //SOBay_FRONTEND_H
