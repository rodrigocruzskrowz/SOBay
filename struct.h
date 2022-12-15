#ifndef SOBAY_STRUCT_H
#define SOBAY_STRUCT_H
#include "utils.h"

typedef struct Utilizador User;
struct Utilizador{
    char nome[MAX_SIZE];
    char password[MAX_SIZE];
    int saldo;
    int valid;
    int pid;
    char word[MAX_SIZE];
};

typedef struct Item Item;
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

#endif //SOBAY_STRUCT_H
