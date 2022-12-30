#ifndef SOBay_BACKEND_H
#define SOBay_BACKEND_H
#include "utils.h"
#include "users_lib.h"
#include <pthread.h>

char *FUSERS;
char *FITEMS;
char *FPROMOTERS;
char *FINIT = "init.txt";

int HEARTBEAT;

int TEMPO;
int PROX_ID;

typedef struct ThreadTempoData TD;
struct ThreadTempoData{
    User *connUt;
    int *nConnUt;
    Item *listIt;
    int *nlistIt;
    pthread_mutex_t *ptrinco;
    pthread_mutex_t *ptrinco_promos;
    int para;
};

typedef struct ThreadRequestRespondeData RR;
struct ThreadRequestRespondeData{
    User *connUt;
    int *nConnUt;
    Item *listIt;
    int *nlistIt;
    int fd_sv_fifo;
    pthread_mutex_t *ptrinco;
    int para;
};

typedef struct Promotor Promotor;
struct Promotor{
    int pid;
    int threadNumber;
    char designacao[MAX_SIZE];
};

typedef struct ThreadPromotorData PD;
struct ThreadPromotorData{
    Promotor *promotor;
    Promo *promocao;
    int *nPromocoes;
    Item *listIt;
    int *nlistIt;
    pthread_mutex_t *ptrinco;
    int para;
};

void stopReadPromotor(int sign, siginfo_t *info);
void stopValidatingLogs(int sign);

//Adiciona nova conexão
void addUserConnection(User ut, User *connUt, int *nusers);

//Remove conexão existente
void removeUserConnection(User ut, User *connUt, int *nusers);

//Carrega/Cria estado da aplicação
void initPlataforma();

//Lê ficheiro de items
//Retorna: 1-Sucesso; 0-Erro;
int readItemsFile(Item *item_lista, int *nitems_lista);

//Recebe os pedidos dos clientes e responde-lhes
void *respondeUsers(void *data);

//Recebe dados dos promotores
void *gerePromotores(void *data);

//Imprime lista de utilizadores com conexão válida estabelecida
void imprimeConnectedUsers(User *dados, int total);

//Imprime lista de promotores ativos
void imprimeActivePromoters(Promotor *dados, int total);

#endif //SOBay_BACKEND_H
