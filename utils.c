#include "utils.h"

void imprimeItems(Item *it, int nitems){
    for(int i=0; i<nitems; i++) {
        printf("\n:::ITEM %d:::\n", i + 1);
        printf("ID: %d\n", it[i].id);
        printf("Item: %s\n", it[i].nome);
        printf("Categoria: %s\n", it[i].categoria);
        if(it[i].promocao.duracao > 0){
            printf("Licitação: %d\n", (it[i].bid - (it[i].promocao.desconto * it[i].bid)));
        }
        else{
            printf("Licitação: %d\n", it[i].bid);
        }
        if(it[i].promocao.duracao > 0){
            printf("Compre já: %d\n", (it[i].buyNow - (it[i].promocao.desconto * it[i].buyNow)));
        }
        else{
            printf("Compre já: %d\n", it[i].buyNow);
        }
        printf("Tempo de venda: %d\n", it[i].tempo);
        if(it[i].promocao.duracao > 0){
            printf("Tempo da promoção: %d\n", it[i].promocao.duracao);
        }
        printf("Vendedor: %s\n", it[i].vendedor);
        printf("Licitador: %s\n", it[i].licitador);
    }
}