#ifndef SOBay_FRONTEND_H
#define SOBay_FRONTEND_H

typedef struct Utilizador ut;
struct Utilizador{
    char *nome;
    char *password;
    int saldo;
    int valid;
    int pid;
};

#endif //SOBay_FRONTEND_H
