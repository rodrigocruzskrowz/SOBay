#ifndef SOBAY_STRUCT_H
#define SOBAY_STRUCT_H
#include "utils.h"

#include "stdio.h"
#include "ctype.h"
#include "string.h"
#include "unistd.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#define MAX_SIZE 256
#define MAX_SIZE_FIFO 32
#define MAX_USERS 20
#define MAX_PROMOTORES 10
#define MAX_ITEMS 30

#define BKND_FIFO "bknd"
#define FRND_FIFO "user_%d"


typedef struct Utilizador User;
struct Utilizador{
    char nome[MAX_SIZE];
    char password[MAX_SIZE];
    int saldo;
    int valid;
    int pid;
};

typedef struct Promocao Promo;
struct Promocao{
    char categoria[MAX_SIZE];
    int desconto;
    int duracao;
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
    Promo promo;
};

typedef struct Cliente_Administrador CA;
struct Cliente_Administrador{
    char word[MAX_SIZE];
    char secWord[MAX_SIZE];
    int number;
    int secNumber;
    User ut;
    Item it[MAX_ITEMS];
};

#endif //SOBAY_STRUCT_H
