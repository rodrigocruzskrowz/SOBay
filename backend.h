#ifndef SOBay_BACKEND_H
#define SOBay_BACKEND_H
#include "utils.h"
#include "users_lib.h"

char *FUSERS;
char *FITEMS;
char *FPROMOTERS;

int TEMPO;
int PROX_ID;

typedef struct Item item;
struct Item{
    int id;
    char nome[MAX_SIZE];
    char categoria[MAX_SIZE];
    int bid;
    int buyNow;
    int tempo;
    char vendedor[MAX_SIZE];
    char licitador[MAX_SIZE];
};

typedef struct Promocao promocao;
struct Promocao{
    char categoria[MAX_SIZE];
    int promo;
    int tempo;
};

#endif //SOBay_BACKEND_H
