#ifndef SOBay_FRONTEND_H
#define SOBay_FRONTEND_H

typedef struct Utilizador ut;
struct Utilizador{
    char nome[100];
    char password[100];
    int saldo;
    int valid;
    int pid;
};

#endif //SOBay_FRONTEND_H
