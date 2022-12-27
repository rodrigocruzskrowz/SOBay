#include "utils.h"

void imprimeItems(Item *it, int nitems){
    for(int i=0; i<nitems; i++) {
        printf("\n:::ITEM %d:::\n", i + 1);
        printf("ID: %d\n", it[i].id);
        printf("Item: %s\n", it[i].nome);
        printf("Categoria: %s\n", it[i].categoria);
        printf("Licitação: %d\n", it[i].bid);
        printf("Compre já: %d\n", it[i].buyNow);
        printf("Tempo de venda: %d\n", it[i].tempo);
        printf("Vendedor: %s\n", it[i].vendedor);
        printf("Licitador: %s\n", it[i].licitador);
    }
}