#include "backend.h"

void addUserConnection(User ut, User *connUt, int *nusers){
    connUt[(*nusers)++] = ut;
}

void removeUserConnection(User ut, User *connUt, int *nusers){
    int encontrado = 0;
    for(int i=0; i<*nusers; i++){
        if(encontrado == 1){
            connUt[i-1] = connUt[i];
            printf("[INFO] Utilizador '%s' removido.\n",ut.nome);
            continue;
        }
        if(connUt[i].pid == ut.pid){
            encontrado = 1;
        }
    }
    (*nusers)--;
}

void initPlataforma(){
    int fd_init = open(FINIT, O_RDONLY);
    if(fd_init == -1){
        printf("\nFicheiro de inicialização não encontrado, a gerar novo ficheiro de inicialização...\n");
        fd_init = open(FINIT, O_WRONLY | O_CREAT,0600);
        char initFile[MAX_SIZE] = "1 1";
        write(fd_init,initFile,strlen(initFile));
        close(fd_init);
        printf("Ficheiro de inicialização gerado!\n\n");
    }
    else{
        char c[MAX_SIZE];
        int tempo, proxID;
        int nbytes;
        while((nbytes = read(fd_init,&c,sizeof(c))) > 0){
            if(sscanf(c,"%d %d",&tempo,&proxID) == 2){
                TEMPO = tempo;
                PROX_ID = proxID;
                printf("TEMPO ATUAL: %d\nPRÓXIMO ID: %d\n\n", TEMPO, PROX_ID);
            }

            if(nbytes == 0)
                break;
        }
    }

    close(fd_init);
}

int readItemsFile(Item *item_lista, int *nitems_lista){
    int fd, cont = 0, arg = 0, nbytes, res, i = 0;
    char str[MAX_SIZE];
    char c;

    fd = open(FITEMS, O_RDONLY);
    if(fd==-1){
        printf("\nFicheiro de items não encontrado! Não existem items listados para venda.\n");
        close(fd);
        return 0;
    }
    else{
        printf("\nInformação do ficheiro: \n");
        while((nbytes = read(fd,&c,1)) >= 0){ //le byte por byte
            //printf("%c",c);
            if(c == '\n' || nbytes == 0){
                str[cont++]='\0';
                if(sscanf(str,"%d %s %s %d %d %d %s %s",
                          &item_lista[i].id,item_lista[i].nome,item_lista[i].categoria,&item_lista[i].bid,
                          &item_lista[i].buyNow,&item_lista[i].tempo,item_lista[i].vendedor,
                          item_lista[i].licitador) == 8){
                    printf("\n:::ITEM %d:::\n",i+1);
                    printf("ID: %d\n", item_lista[i].id);
                    printf("Item: %s\n", item_lista[i].nome);
                    printf("Categoria: %s\n", item_lista[i].categoria);
                    printf("Licitação: %d\n", item_lista[i].bid);
                    printf("Compre já: %d\n", item_lista[i].buyNow);
                    printf("Tempo de venda: %d\n", item_lista[i].tempo);
                    printf("Vendedor: %s\n", item_lista[i].vendedor);
                    printf("Licitador: %s\n", item_lista[i].licitador);

                    (*nitems_lista)++;
                    //printf("nlista: %d\n",nitems_lista);
                    PROX_ID = item_lista[i].id;
                    PROX_ID++;
                    //printf("prox_id: %d\n",PROX_ID);

                    if(i < MAX_ITEMS)
                        i++;
                    else
                        break;

                    memset(str,0,MAX_SIZE);
                    cont = 0;

                }
                else{
                    memset(str,0,MAX_SIZE);
                    cont = 0;
                }

                if(nbytes == 0)
                    break;
            }
            else{
                str[cont++] = c;
            }
        }
    }
    close(fd);
    return 1;
}

void *incTempoInfo(void *data){
    TD *ttd = data;

    while(ttd->para == 1){
        //Incrementa tempo
        sleep(1);
        pthread_mutex_lock(ttd->ptrinco);
        pthread_mutex_lock(ttd->ptrinco_promos);
        TEMPO++;

        //Decrementa tempo de items em leilao
        for(int i=0;i<*ttd->nlistIt;i++){

            //Decrementa tempo do item
            (ttd->listIt[i].tempo)--;

            //Decrementa tempo da promoções (?)
            if(ttd->listIt[i].promocao.duracao > 0)
                (ttd->listIt[i].promocao.duracao)--;

            if(ttd->listIt[i].tempo == 0){
                //Avisa todos os utilizadores que o item X expirou
                for(int j=0;j<*ttd->nConnUt;j++){
                    char res_cli_fifo[MAX_SIZE_FIFO];
                    int fd_cli_fifo;
                    CA comm;
                    sprintf(res_cli_fifo, FRND_FIFO, ttd->connUt->pid);
                    fd_cli_fifo = open(res_cli_fifo, O_WRONLY);
                    strcpy(comm.word,"ITEXPIRED");
                    strcpy(comm.secWord,ttd->listIt[i-1].nome);
                    write(fd_cli_fifo, &comm, sizeof(CA));
                    close(fd_cli_fifo);
                }

                for(int x=i;x<*ttd->nlistIt;x++)
                    ttd->listIt[x] = ttd->listIt[x+1];

                (*ttd->nlistIt)--;
            }
        }

        pthread_mutex_unlock(ttd->ptrinco_promos);
        pthread_mutex_unlock(ttd->ptrinco);
    }
    pthread_exit(NULL);
}

void *respondeUsers(void *data){
    RR *trr = data;
    CA comm;
    char res_cli_fifo[MAX_SIZE_FIFO];
    int fd_cli_fifo;

    //Atende pedidos
    while(trr->para == 1){
        int lido = read(trr->fd_sv_fifo,&comm,sizeof(CA));
        if(lido == sizeof(CA)){
            //ESPERAR PELO TRINCO (BLOQUEAR)
            pthread_mutex_lock(trr->ptrinco);

            if(strcmp(comm.word,"LOGIN")==0){
                //Verifica
                int encontrou = 0;
                for (int i = 0; i < *trr->nConnUt; i++) {
                    if (trr->connUt[i].pid == comm.ut.pid) {
                        encontrou = 1;
                        printf("[ERROR] O utilizador %s já está na tabela de utilizadores conectados.\n", comm.ut.nome);
                    }
                }

                if (encontrou == 0) {
                    comm.ut.valid = isUserValid(comm.ut.nome, comm.ut.password);
                    if (comm.ut.valid == 0) {
                        sprintf(res_cli_fifo, FRND_FIFO, comm.ut.pid);
                        fd_cli_fifo = open(res_cli_fifo, O_WRONLY);
                        write(fd_cli_fifo, &comm, sizeof(CA));
                        close(fd_cli_fifo);
                        printf("[INFO] Tentativa de acesso com credenciais erradas.\n");
                    } else if (comm.ut.valid == 1) {
                        comm.ut.saldo = getUserBalance(comm.ut.nome);
                        comm.number = HEARTBEAT;
                        sprintf(res_cli_fifo, FRND_FIFO, comm.ut.pid);
                        fd_cli_fifo = open(res_cli_fifo, O_WRONLY);
                        write(fd_cli_fifo, &comm, sizeof(CA));
                        close(fd_cli_fifo);
                        printf("[INFO] O utilizador '%s' fez login na plataforma.\n", comm.ut.nome);
                        addUserConnection(comm.ut, trr->connUt, trr->nConnUt);
                    } else {
                        printf("[ERRO] %s\n", getLastErrorText());
                    }
                }
            }

            else if(strcmp(comm.word,"CRIAR")==0){
                //Lê dados do item

                printf("\nItem recebido:\nId: %d\nNome: %s\nCategoria: %s\nPreço atual: %d\nPreço compre já: %d\nTempo até fim de leilão: %d\nVendedor: %s\nLicitador: %s\n\n",
                       comm.it[0].id,comm.it[0].nome,comm.it[0].categoria,comm.it[0].bid,comm.it[0].buyNow,comm.it[0].tempo,comm.it[0].vendedor,comm.it[0].licitador);

                //Coloca item à venda (adiciona à lista de items)
                trr->listIt[*trr->nlistIt].id = PROX_ID;
                strcpy(trr->listIt[*trr->nlistIt].nome, comm.it[0].nome);
                strcpy(trr->listIt[*trr->nlistIt].categoria, comm.it[0].categoria);
                trr->listIt[*trr->nlistIt].bid = comm.it[0].bid;
                trr->listIt[*trr->nlistIt].buyNow = comm.it[0].buyNow;
                trr->listIt[*trr->nlistIt].tempo = comm.it[0].tempo;
                strcpy(trr->listIt[*trr->nlistIt].vendedor, comm.it[0].vendedor);
                strcpy(trr->listIt[*trr->nlistIt].licitador, comm.it[0].licitador);
                PROX_ID++;
                (*trr->nlistIt)++;

                //Informa cliente
                strcpy(comm.word,"INSERIDO");
                comm.number = *trr->nlistIt;
                comm.secNumber = trr->listIt[(*trr->nlistIt)-1].id;
                sprintf(res_cli_fifo, FRND_FIFO, comm.ut.pid);
                fd_cli_fifo = open(res_cli_fifo, O_WRONLY);
                write(fd_cli_fifo, &comm, sizeof(CA));
                close(fd_cli_fifo);

                printf(":::::LISTA DE ITEMS:::::");
                for(int i=0; i<*trr->nlistIt; i++){
                    printf("\n:::ITEM %d:::\n",i+1);
                    printf("ID: %d\n", trr->listIt[i].id);
                    printf("Item: %s\n", trr->listIt[i].nome);
                    printf("Categoria: %s\n", trr->listIt[i].categoria);
                    printf("Licitação: %d\n", trr->listIt[i].bid);
                    printf("Compre já: %d\n", trr->listIt[i].buyNow);
                    printf("Tempo de venda: %d\n", trr->listIt[i].tempo);
                    printf("Vendedor: %s\n", trr->listIt[i].vendedor);
                    printf("Licitador: %s\n", trr->listIt[i].licitador);
                }
            }

            else if(strcmp(comm.word,"LISTAR")==0){

                strcpy(comm.word,"ENVIADO");
                comm.number = *trr->nlistIt;
                sprintf(res_cli_fifo, FRND_FIFO, comm.ut.pid);
                fd_cli_fifo = open(res_cli_fifo, O_WRONLY);

                //Add items à estrutura de comunicação
                int itemsEnviados = 0;
                for(int i=0; i<*trr->nlistIt; i++){
                    comm.it[i] = trr->listIt[i];
                    itemsEnviados++;
                }

                //Enviar lista de items
                int n = write(fd_cli_fifo,&comm,sizeof(CA));
                printf("Enviei %d items.\n",itemsEnviados);

                close(fd_cli_fifo);
            }

            else if(strcmp(comm.word,"LISTCAT")==0){
                //Verifica se existem items com a categoria recebida
                int itemsAEnviar = 0;
                for(int i=0;i<*trr->nlistIt;i++){
                    if(strcmp(trr->listIt[i].categoria,comm.secWord)==0){
                        comm.it[itemsAEnviar++] = trr->listIt[i];
                    }
                }

                sprintf(res_cli_fifo, FRND_FIFO, comm.ut.pid);
                fd_cli_fifo = open(res_cli_fifo, O_WRONLY);

                if(itemsAEnviar > 0){
                    //Enviar sucesso na receção do comando
                    strcpy(comm.word,"ENVCAT");
                    comm.number = itemsAEnviar;

                    //Enviar items da categoria recebida
                    write(fd_cli_fifo,&comm,sizeof(CA));
                    printf("Enviei %d items.\n",itemsAEnviar);
                }
                else{
                    //Enviar erro (nenhum item a listar)
                    strcpy(comm.word,"ENVCAT");
                    comm.number = 0;
                    write(fd_cli_fifo,&comm,sizeof(CA));
                }

                close(fd_cli_fifo);
            }

            else if(strcmp(comm.word,"LISTSEL")==0){
                //Verifica se existem items do vendedor recebida
                int itemsAEnviar = 0;
                for(int i=0;i<*trr->nlistIt;i++){
                    if(strcmp(trr->listIt[i].vendedor,comm.secWord)==0){
                        comm.it[itemsAEnviar++] = trr->listIt[i];
                    }
                }

                sprintf(res_cli_fifo, FRND_FIFO, comm.ut.pid);
                fd_cli_fifo = open(res_cli_fifo, O_WRONLY);

                if(itemsAEnviar > 0){
                    //Enviar sucesso na receção do comando
                    strcpy(comm.word,"ENVSEL");
                    comm.number = itemsAEnviar;

                    //Enviar items da categoria recebida
                    write(fd_cli_fifo,&comm,sizeof(CA));
                    printf("Enviei %d items.\n",itemsAEnviar);
                }
                else{
                    //Enviar erro (nenhum item a listar)
                    strcpy(comm.word,"ENVSEL");
                    comm.number = 0;
                    write(fd_cli_fifo,&comm,sizeof(CA));
                }

                close(fd_cli_fifo);
            }

            else if(strcmp(comm.word,"LISTVAL")==0){
                //Verifica se existem items ate ao valor recebido
                int itemsAEnviar = 0;
                for(int i=0;i<*trr->nlistIt;i++){
                    if(trr->listIt[i].bid <= comm.number){
                        comm.it[itemsAEnviar++] = trr->listIt[i];
                    }
                }

                sprintf(res_cli_fifo, FRND_FIFO, comm.ut.pid);
                fd_cli_fifo = open(res_cli_fifo, O_WRONLY);

                if(itemsAEnviar > 0){
                    //Enviar sucesso na receção do comando
                    strcpy(comm.word,"ENVVAL");
                    comm.number = itemsAEnviar;

                    //Enviar items da categoria recebida
                    write(fd_cli_fifo,&comm,sizeof(CA));
                    printf("Enviei %d items.\n",itemsAEnviar);
                }
                else{
                    //Enviar erro (nenhum item a listar)
                    strcpy(comm.word,"ENVVAL");
                    comm.number = 0;
                    write(fd_cli_fifo,&comm,sizeof(CA));
                }

                close(fd_cli_fifo);
            }

            else if(strcmp(comm.word,"LISTIM")==0){
                //Verifica se existem items ate ao valor recebido
                int itemsAEnviar = 0;
                for(int i=0;i<*trr->nlistIt;i++){
                    if((TEMPO + trr->listIt[i].tempo) >= comm.number){
                        comm.it[itemsAEnviar++] = trr->listIt[i];
                    }
                }

                sprintf(res_cli_fifo, FRND_FIFO, comm.ut.pid);
                fd_cli_fifo = open(res_cli_fifo, O_WRONLY);

                if(itemsAEnviar > 0){
                    //Enviar sucesso na receção do comando
                    strcpy(comm.word,"ENVTIM");
                    comm.number = itemsAEnviar;

                    //Enviar items da categoria recebida
                    write(fd_cli_fifo,&comm,sizeof(CA));
                    printf("Enviei %d items.\n",itemsAEnviar);
                }
                else{
                    //Enviar erro (nenhum item a listar)
                    strcpy(comm.word,"ENVTIM");
                    comm.number = 0;
                    write(fd_cli_fifo,&comm,sizeof(CA));
                }

                close(fd_cli_fifo);
            }

            else if(strcmp(comm.word,"TIME")==0){
                sprintf(res_cli_fifo, FRND_FIFO, comm.ut.pid);
                fd_cli_fifo = open(res_cli_fifo, O_WRONLY);

                //Enviar tempo atual
                strcpy(comm.word,"ENVTIME");
                comm.number = TEMPO;
                write(fd_cli_fifo,&comm,sizeof(CA));

                close(fd_cli_fifo);
            }

            else if(strcmp(comm.word,"BUY")==0){
                //TODO: Alterar para utilizar a lista de utilizadores e não fazer a leitura diretamente do ficheiro.
                //word-IDENTIFICATION WORD; secWord-USERNAME; number-ITEM ID; secNumber-VALOR
                int encontrou = 0;
                int compra = 0;
                int novoSaldo;

                sprintf(res_cli_fifo, FRND_FIFO, comm.ut.pid);
                fd_cli_fifo = open(res_cli_fifo, O_WRONLY);

                for(int i=0; i<*trr->nlistIt;i++){

                    //Reorganiza lista de items (em caso de compra do item)
                    if(compra == 1){
                        trr->listIt[i-1] = trr->listIt[i];
                        continue;
                    }

                    if(trr->listIt[i].id == comm.number){
                        encontrou = 1;
                        if(trr->listIt[i].buyNow != 0 && trr->listIt[i].buyNow <= comm.secNumber){
                            //Compra
                            //Verifica se user que licitou tem saldo suficiente
                            int saldo = getUserBalance(comm.secWord);
                            if(saldo >= comm.secNumber){
                                //Aprova compra
                                novoSaldo = (saldo - comm.secNumber);
                                updateUserBalance(comm.secWord,novoSaldo);
                                saveUsersFile(FUSERS);
                                compra = 1;
                            }else{
                                //Saldo insuficiente
                                strcpy(comm.word,"ERRSALDO");
                                comm.number = saldo;
                                write(fd_cli_fifo,&comm,sizeof(CA));
                            }
                        }
                        else if(trr->listIt[i].bid < comm.secNumber){
                            //Licitação
                            //Verifica se item tem licitador
                            if(strcmp(trr->listIt[i].licitador,"-")!=0){
                                //Restituir saldo do ultimo licitador
                                int res = updateUserBalance(trr->listIt[i].licitador,trr->listIt[i].bid);
                                if(res == -1){
                                    printf("[WARNING] Não foi possível restituir o saldo do licitador anterior.\n");
                                    printf("[ERRO] %s\n",getLastErrorText());
                                }
                                else if(res == 0){
                                    printf("[WARNING] O utilizador '%s' não foi encontrado. Não foi possível restituir o seu saldo.\n",trr->listIt[i].licitador);
                                }
                                else{
                                    saveUsersFile(FUSERS);
                                    printf("[INFO] Saldo do utilizador '%s' restituido.\n",trr->listIt[i].licitador);
                                }
                            }
                            int saldo = getUserBalance(comm.secWord);
                            if(saldo >= comm.secNumber){
                                //Aprova licitação
                                novoSaldo = (saldo - comm.secNumber);
                                updateUserBalance(comm.secWord,novoSaldo);
                                saveUsersFile(FUSERS);
                                strcpy(trr->listIt[i].licitador,comm.secWord);

                                //Envia para frontend
                                strcpy(comm.word,"BIDDED");
                                comm.number = novoSaldo;
                                write(fd_cli_fifo,&comm,sizeof(CA));
                            }
                            else{
                                //Saldo insuficiente
                                strcpy(comm.word,"ERRSALDO");
                                comm.number = saldo;
                                write(fd_cli_fifo,&comm,sizeof(CA));
                            }
                        }
                        else{
                            strcpy(comm.word,"ERRVAL");
                            write(fd_cli_fifo,&comm,sizeof(CA));
                        }
                    }
                }

                if(compra == 1){
                    *trr->nlistIt--;

                    //Envia para frontend
                    strcpy(comm.word,"BOUGHT");
                    comm.number = novoSaldo;
                    write(fd_cli_fifo,&comm,sizeof(CA));
                }

                if(encontrou == 0){
                    //Envia para frontend
                    strcpy(comm.word,"ERRID");
                    write(fd_cli_fifo,&comm,sizeof(CA));
                }

                close(fd_cli_fifo);
            }

            else if(strcmp(comm.word,"CASH")==0){
                sprintf(res_cli_fifo, FRND_FIFO, comm.ut.pid);
                fd_cli_fifo = open(res_cli_fifo, O_WRONLY);

                //Enviar saldo
                comm.number = getUserBalance(comm.secWord);
                strcpy(comm.word,"ENVCASH");
                write(fd_cli_fifo,&comm,sizeof(CA));

                close(fd_cli_fifo);
            }

            else if(strcmp(comm.word,"ADDMONEY")==0){
                sprintf(res_cli_fifo, FRND_FIFO, comm.ut.pid);
                fd_cli_fifo = open(res_cli_fifo, O_WRONLY);

                //Enviar confirmação carregamento
                int saldo = getUserBalance(comm.secWord);
                int novoSaldo = saldo + comm.number;
                //printf("PEDIDO: %d %s %d %d",comm.number,comm.secWord,saldo,(saldo + comm.number));
                int res = updateUserBalance(comm.secWord,novoSaldo);
                if(res == -1){
                    printf("[WARNING] Não foi possível restituir o saldo do licitador anterior.\n");
                    printf("[ERRO] %s\n",getLastErrorText());
                }
                else if(res == 0){
                    printf("[WARNING] O utilizador '%s' não foi encontrado. Não foi possível atualizar o seu saldo.\n",comm.secWord);
                }
                else{
                    saveUsersFile(FUSERS);
                    printf("[INFO] Adicionadas %d SOCOins ao utilizador '%s'.\n",comm.number,comm.secWord);
                }

                strcpy(comm.word,"ENVADDMONEY");
                comm.number = novoSaldo;
                write(fd_cli_fifo,&comm,sizeof(CA));

                close(fd_cli_fifo);
            }

            else if(strcmp(comm.word,"EXIT")==0){
                printf("[INFO] O cliente %d informou que ia sair.\n",comm.ut.pid);
                removeUserConnection(comm.ut,trr->connUt,trr->nConnUt);
            }

            else if(strcmp(comm.word,"SHUTDOWN")==0){
                printf("[INFO] Pedido de encerramento recebido.\n");
            }
        }
        //ESPERAR PELO TRINCO (DESBLOQUEAR)
        pthread_mutex_unlock(trr->ptrinco);
    }
    pthread_exit(NULL);
}

void *gerePromotores(void *data){
    PD *tpd = data;

    int fd, cont = 0, arg = 0, nbytes, estado;
    char str[MAX_SIZE], c;

    int res;
    int canal[2];
    pipe(canal);

    tpd->promotor->pid = fork();
    if (tpd->promotor->pid == 0) {
        close(1);
        dup(canal[1]);
        close(canal[1]);
        close(canal[0]);

        execl(tpd->promotor->designacao, tpd->promotor->designacao, NULL);

        printf("[ERRO] Erro no 'execl' da thread %d %s.\n",tpd->promotor->threadNumber,tpd->promotor->designacao);
        pthread_exit(NULL);
    }

    close(canal[1]);
    fflush(stdout);
    memset(str,0,MAX_SIZE);
    while (tpd->para == 1) {
        res = read(canal[0], str, sizeof(str)-1);
        if (res > 0) {
            str[res] = '\0';
            printf("O promotor enviou-me: %s", str);

            char s[MAX_SIZE];
            cont = 0;
            pthread_mutex_lock(tpd->ptrinco);
            for(int itr=0; itr<strlen(str);itr++){
                if(str[itr] != '\n' && str[itr] != ' '){
                    s[cont++] = str[itr];
                }
                else{
                    s[cont] = '\0';
//                                printf("%s",s);
//                                memset(s,0,MAX_SIZE);
//                                printf("\nESPAÇO//bN\n");
                    cont = 0;
                    switch (arg) {
                        case 0:{
                            strcpy(tpd->promocao[*tpd->nPromocoes].categoria,s);
                            memset(s,0,MAX_SIZE);
                            break;
                        }
                        case 1:{
                            tpd->promocao[*tpd->nPromocoes].desconto = atoi(s);
                            memset(s,0,MAX_SIZE);
                            break;
                        }
                        case 2:{
                            tpd->promocao[*tpd->nPromocoes].duracao = atoi(s);
                            memset(s,0,MAX_SIZE);
                            arg=-1;
                            break;
                        }
                        default: {
                            printf("[ERRO] Fora dos limites de atributos.\n");
                            break;
                        }
                    }
                    arg++;
                    if(itr == strlen(str)-1){
                        printf("\n:::PROMOÇÃO %d:::\n",(*tpd->nPromocoes)+1);
                        printf("Categoria: %s\n", tpd->promocao[*tpd->nPromocoes].categoria);
                        printf("Desconto: %d\n", tpd->promocao[*tpd->nPromocoes].desconto);
                        printf("Duração: %d\n\n", tpd->promocao[*tpd->nPromocoes].duracao);
                        (*tpd->nPromocoes)++;

                        for(int i=0;i<*tpd->nlistIt;i++){
                            if(strcmp(tpd->listIt->categoria,tpd->promocao->categoria)==0){
                                strcpy(tpd->listIt->promocao.categoria,tpd->promotor->designacao); //Guarda o promotor no campo categoria
                                tpd->listIt->promocao.duracao = tpd->promocao->duracao;
                                tpd->listIt->promocao.desconto = tpd->promocao->desconto;
                            }
                        }
                    }
                }
            }
            pthread_mutex_unlock(tpd->ptrinco);
        }
    }
    union sigval sv;
    sv.sival_int = 0;
    sigqueue(tpd->promotor->pid, SIGUSR1, sv);
    wait(&estado);

    pthread_exit(NULL);
}

void imprimeConnectedUsers(User *dados, int total){
    if(total > 0){
        printf("\n[INFO] Utilizadores ligados: %d\n",total);
        for(int i=0; i<total; i++){
            printf(" Utilizador %d: %s\n", i+1,dados[i].nome);
        }
    }
    else{
        printf("[INFO] Não existem atualmente utilizadores ligados.\n");
    }
}

void imprimeActivePromoters(Promotor *dados, int total){
    if(total > 0){
        printf("\n[INFO] Promotores ativos: %d\n",total);
        for(int i=0; i<total; i++){
            printf(" Promotor %d: %s\n", i+1,dados[i].designacao);
        }
    }
    else{
        printf("[INFO] Não existem promotores ativos.\n");
    }
}

int main() {

    initPlataforma();

    User connectedUsers[MAX_USERS];
    int nConnectedUsers = 0;
    Item item_lista[MAX_ITEMS];
    int nitems_lista = 0;
    Promotor promotor_lista[MAX_PROMOTORES];
    int npromotor_lista = 0;
    Promo promo[MAX_SIZE];
    int npromos = 0;
    int fd_sv_fifo;

    //Verifica se backend já está em execução
    if(access(BKND_FIFO,F_OK) == 0){
        printf("O BACKEND já está em execução!\n");
        exit(1);
    }

    //Verifica se a variavel de ambiente FUSERS existe
    if(getenv("FUSERS") == NULL){
        printf("A variável de ambiente 'FUSERS' não foi definida.\n");
//        FUSERS = "../users.txt";
        exit(1);
    }
    else{
        FUSERS = getenv("FUSERS");
        printf("\nVariável de ambiente 'FUSERS' = %s\n",FUSERS);
    }

    //Verifica se a variavel de ambiente FPROMOTERS existe
    if(getenv("FPROMOTERS") == NULL){
        printf("A variável de ambiente 'FPROMOTERS' não foi definida.\n");
//        FPROMOTERS = "../promotores.txt";
        exit(1);
    }
    else{
        FPROMOTERS = getenv("FPROMOTERS");
        printf("\nVariável de ambiente 'FPROMOTERS' = %s\n",FPROMOTERS);
    }

    //Verifica se a variavel de ambiente FITEMS existe
    if(getenv("FITEMS") == NULL){
        printf("A variável de ambiente 'FITEMS' não foi definida.\n");
//        FITEMS = "../items.txt";
        exit(1);
    }
    else{
        FITEMS = getenv("FITEMS");
        printf("\nVariável de ambiente 'FITEMS' = %s\n",FITEMS);
    }

    //Verifica se a variavel de ambiente HEARTBEAT existe
    if(getenv("HEARTBEAT") == NULL){
        printf("A variável de ambiente 'HEARTBEAT' não foi definida.\n");
//        HEARTBEAT = 10;
        exit(1);
    }
    else{
        HEARTBEAT = atoi(getenv("HEARTBEAT"));
        printf("Variável de ambiente 'HEARTBEAT' = %d\n\n",HEARTBEAT);
    }

    //Acede ao ficheiro de utilizadores
    int nUtilizadores = loadUsersFile(FUSERS);
    printf("\nNum. Utilizadores no ficheiro: %d\n",nUtilizadores);

    int readSuccess = readItemsFile(item_lista,&nitems_lista);
    if(readSuccess == 0){
        exit(1);
    }

    //Cria BACKEND FIFO
    mkfifo(BKND_FIFO,0600);
    fd_sv_fifo = open(BKND_FIFO,O_RDWR);

    //Cria mutex - thread
    pthread_mutex_t trinco, trinco_promocoes;
    pthread_mutex_init(&trinco,NULL);
    pthread_mutex_init(&trinco_promocoes,NULL);
    pthread_t threadRR, threadT, threadPromotor[MAX_PROMOTORES];
    RR rrdata;
    TD tddata;
    PD pddata[MAX_PROMOTORES];

    rrdata.connUt = connectedUsers;
    rrdata.nConnUt = &nConnectedUsers;
    rrdata.listIt = item_lista;
    rrdata.nlistIt = &nitems_lista;
    rrdata.ptrinco = &trinco;
    rrdata.fd_sv_fifo = fd_sv_fifo;
    rrdata.para = 1;

    int ver = pthread_create(&threadRR, NULL, respondeUsers, &rrdata);
    if(ver != 0){
        printf("[ERROR] Erro ao criar a thread %d (validação de login).\n",ver);
        exit(1);
    }

    tddata.connUt = connectedUsers;
    tddata.nConnUt = &nConnectedUsers;
    tddata.listIt = item_lista;
    tddata.nlistIt = &nitems_lista;
    tddata.ptrinco = &trinco;
    tddata.ptrinco_promos = &trinco_promocoes;
    tddata.para = 1;

    ver = pthread_create(&threadT, NULL, incTempoInfo, &tddata);
    if(ver != 0){
        printf("[ERROR] Erro ao criar a thread %d (gestor de tempo).\n",ver);
        exit(1);
    }

    //Espera comando
    char str[MAX_SIZE] = "";
    char comando[6][MAX_SIZE];
    while (strcmp(comando[0],"close")) {
        int i = 0, j = 0, c = 0;
        printf("\ncommand > ");
        fflush(stdout);
        setbuf(stdin, NULL); //stdin flush
        fgets(str, sizeof str, stdin);

        for (i = 0; i <= (strlen(str)); i++) {
            if (str[i] == ' ' || str[i] == '\n') {
                comando[c][j] = '\0';
                c++;
                j = 0;
            } else {
                comando[c][j] = str[i];
                j++;
            }
        }

        //Comandos de administrador
        if (strcmp(comando[0], "users") == 0) {
            if (c == 1) {
                //Listar utilizadores a usar a plataforma
//                printf("Comando válido!\n");
                imprimeConnectedUsers(connectedUsers,nConnectedUsers);

            } else {
                printf("[WARNING] O comando inserido não é válido.\n");
                printf("Formato esperado:\n\tusers\n\n");
            }
        } else if (strcmp(comando[0], "list") == 0) {
            if (c == 1) {
                //Listar items à venda
//                printf("Comando válido!\n");
                imprimeItems(item_lista,nitems_lista);
            } else {
                printf("[WARNING] O comando inserido não é válido.\n");
                printf("Formato esperado:\n\tlist\n\n");
            }
        } else if (strcmp(comando[0], "kick") == 0) {
            if (c == 2) {
                //Verificar se utilizador existe
                for(int i=0;i<nConnectedUsers;i++){
                    if(strcmp(connectedUsers[i].nome,comando[1])==0){
                        CA comm;
                        char res_cli_fifo[MAX_SIZE_FIFO];
                        int fd_cli_fifo;
                        sprintf(res_cli_fifo, FRND_FIFO, connectedUsers[i].pid);
                        fd_cli_fifo = open(res_cli_fifo, O_WRONLY);
                        strcpy(comm.word,"UTKICK");
                        write(fd_cli_fifo, &comm, sizeof(CA));
                        close(fd_cli_fifo);

                        removeUserConnection(comm.ut,connectedUsers,&nConnectedUsers);
                    }
                }

                //Expulsar o utilizador
//                printf("Comando válido!\n");

            } else {
                printf("[WARNING] O comando inserido não é válido.\n");
                printf("Formato esperado:\n\tkick <username>\n\n");
            }
        } else if (strcmp(comando[0], "prom") == 0) {
            if (c == 1) {
                //Listar promotores ativos
//                printf("Comando válido!\n");
                imprimeActivePromoters(promotor_lista,npromotor_lista);
            } else {
                printf("[WARNING] O comando inserido não é válido.\n");
                printf("Formato esperado:\n\tprom\n\n");
            }
        } else if (strcmp(comando[0], "reprom") == 0) {
            if (c == 1) {
                //Atualizar promotores
//                printf("Comando válido!\n");
                int fdPromo, cont = 0, nbytes;
                char promotoresLidos[MAX_PROMOTORES][MAX_SIZE];
                int nPromotoresLidos = 0;

                int livre = -1; //1ª posição livre

                //Lê promotores
                fdPromo = open(FPROMOTERS, O_RDONLY);
                if(fdPromo==-1){
                    printf("\n[ERROR] Ficheiro de promotores não encontrado.\n");
                    break;
                }
                else{
                    i = 0;
                    memset(str,0,MAX_SIZE);
                    while((nbytes = read(fdPromo,&c,1)) >= 0){ //le byte por byte
                        //printf("%c",c);
                        if(c == '\n' || nbytes == 0){
                            str[cont++]='\0';
                            if(sscanf(str,"%s",promotoresLidos[i]) == 1){
                                //printf("%d %s\n",i+1,promotoresLidos[i]);
                                nPromotoresLidos++;

                                //--------------------TESTES----------------------------------------------------------
//                                if(i == 0)
//                                strcpy(promotoresLidos[i],"../promospot");
//                                if(i == 1)
//                                strcpy(promotoresLidos[i],"../promotor_oficial");
//                                if(i == 2)
//                                strcpy(promotoresLidos[i],"../black_friday");
                                //------------------------------------------------------------------------------------

                                if(i < MAX_PROMOTORES)
                                    i++;
                                else
                                    break;

                                memset(str,0,MAX_SIZE);
                                cont = 0;

                            }
                            else{
                                memset(str,0,MAX_SIZE);
                                cont = 0;
                            }

                            if(nbytes == 0)
                                break;
                        }
                        else{
                            str[cont++] = c;
                        }
                    }
                }

                //Verifica se está em execução
                int reorganiza = 0;
                if(npromotor_lista != 0){
                    int comp = 0;
                    for(i=0; i < npromotor_lista; i++){

                        if(reorganiza == 1) {
                            item_lista[i - 1] = item_lista[i];
                            nitems_lista--;
                            reorganiza = 0;
                        }

                        for(j=0; j < nPromotoresLidos; j++){

                            if(promotor_lista[j].designacao != promotoresLidos[i]){
                                comp++;
                            }

                            if(comp == npromotor_lista){
                                comp = 0;

                                //Encerra promotor
                                //TODO: ENCERRAR A THREAD
                                int fd_prom;
                                char res_prom_fifo[MAX_SIZE_FIFO];

                                pthread_join(threadPromotor[promotor_lista[j].threadNumber],NULL);

                                //Remove da lista
                                reorganiza = 1;
                            }
                        }
                    }
                }

                if(reorganiza == 1){
                    nitems_lista--;
                }

                //Cria thread para executar promotores na lista
                for(i=0; i<nPromotoresLidos; i++){
                    if(access(promotoresLidos[i],F_OK)!=0) {
                        printf("[ERRO] Não foi possível executar o promotor '%s'. Verifique se o nome do ficheiro está correto.\n",
                               promotoresLidos[i]);
                        continue;
                    }
                    for(j=0; j<MAX_PROMOTORES; j++){
                        if(livre == -1 && strcmp(promotor_lista[j].designacao,"") == 0){
                            livre = j;
                            break;
                        }
                    }
                    if(livre >= 0){
                        //Lança promotor
                        strcpy(promotor_lista[livre].designacao,promotoresLidos[i]);
                        promotor_lista[livre].threadNumber = livre;
                        npromotor_lista++;

                        pddata[livre].promotor = &promotor_lista[livre];
                        pddata[livre].promocao = promo;
                        pddata[livre].nPromocoes = &npromos;
                        pddata[livre].listIt = item_lista;
                        pddata[livre].nlistIt = &nitems_lista;
                        pddata[livre].para = 1;
                        pddata[livre].ptrinco = &trinco_promocoes;

                        ver = pthread_create(&threadPromotor[livre], NULL, gerePromotores, &pddata[livre]);
                        if(ver != 0){
                            printf("[ERROR] Erro ao criar a thread %d (execução do promotor).\n",ver);
                            break;
                        }
                    }

                    livre = -1;
                }

                close(fdPromo);
            } else {
                printf("[WARNING] O comando inserido não é válido.\n");
                printf("Formato esperado:\n\treprom\n\n");
            }
        }

        else if (strcmp(comando[0], "cancel") == 0) {
            if (c == 2) {
                //Verificar se o executavel existe

                //Cancelar promotor
//                printf("Comando válido!\n");

                int elimina = 0;
                for(int it=0;it<npromotor_lista;it++){
                    if(elimina == 1){
                        promotor_lista[it-1] = promotor_lista[it];
                    }

                    if(strcmp(promotor_lista[it].designacao,comando[1])==0){
                        pddata[promotor_lista[it].threadNumber].para = 0;
                        pthread_join(threadPromotor[promotor_lista[it].threadNumber],NULL);

                        for(int x=0; x<nitems_lista; x++){
                            if(strcmp(item_lista[x].promocao.categoria,comando[1])==0){
                                strcpy(item_lista[x].promocao.categoria,"");
                                item_lista[x].promocao.desconto = 0;
                                item_lista[x].promocao.duracao = 0;
                            }
                        }

                        elimina = 1;
                    }
                }

                if(elimina == 1){
                    npromotor_lista--;
                }

            } else {
                printf("[WARNING] O comando inserido não é válido.\n");
                printf("Formato esperado:\n\tcancel <nome-do-executável-do-promotor>\n\n");
            }
        }

        else if (strcmp(comando[0], "close") == 0) {
            if (c == 1) {
                CA comm;
                char res_cli_fifo[MAX_SIZE_FIFO];
                int fd_cli_fifo;

                //Notifica frontends
                strcpy(comm.word,"SHUTDOWNALL");
                for(int i=0; i<nConnectedUsers; i++){
                    sprintf(res_cli_fifo, FRND_FIFO, connectedUsers->pid);
                    fd_cli_fifo = open(res_cli_fifo, O_WRONLY);
                    write(fd_cli_fifo,&comm,sizeof(CA));
                    close(fd_cli_fifo);
                }

                //Encerra plataforma
                break;
            } else {
                printf("[WARNING] O comando inserido não é válido.\n");
                printf("Formato esperado:\n\tclose\n\n");
            }
        } else {
            if (strcmp(comando[0], "") != 0) {
                printf("[WARNING] O comando inserido não é válido.\n");
            }
        }
        i = 0, j = 0, c = 0;
    }

    for(int i=0; i<nConnectedUsers; i++){
        printf("%s %d\n",connectedUsers[i].nome,connectedUsers[i].saldo);
    }

    //Termina threads dos promotores
    for(int i=0;i<npromotor_lista;i++){
        int fdPromo;
        char res_prom_fifo[MAX_SIZE_FIFO];

        int n = write(fd_sv_fifo,&pddata,sizeof(PD));
        if(n == sizeof(CA)){
            printf("[INFO] Enviei %s\n",pddata[i].promocao->categoria);
        }
        pthread_join(threadPromotor[promotor_lista[i].threadNumber], NULL);
    }

    //Termina thread request/response
    rrdata.para = 0;
    CA end;
    strcpy(end.word,"SHUTDOWN");
    int n = write(fd_sv_fifo,&end,sizeof(CA));
    if(n == sizeof(CA)){
        printf("[INFO] Enviei %s\n",end.word);
    }
    pthread_join(threadRR, NULL);

    pthread_mutex_destroy(&trinco_promocoes);
    pthread_mutex_destroy(&trinco);

    //Guarda ficheiro de items
    remove(FITEMS);
    int fd_items = open(FITEMS, O_WRONLY | O_CREAT,0600);
    for(int i=0; i<nitems_lista;i++){
        char texto[1072];
        sprintf(texto,"%d %s %s %d %d %d %s %s\n",item_lista[i].id,item_lista[i].nome,item_lista[i].categoria,
                item_lista[i].bid,item_lista[i].buyNow,item_lista[i].tempo,item_lista[i].vendedor,item_lista[i].licitador);
        write(fd_items,&texto, strlen(texto));
    }
    close(fd_items);

    //Guarda estado da plataforma (tempo e id)
    remove(FINIT);
    int fd_init = open(FINIT, O_WRONLY | O_CREAT,0600);
    char texto[MAX_SIZE];
    sprintf(texto,"%d %d",TEMPO, PROX_ID);
    write(fd_init,&texto, strlen(texto));
    close(fd_init);

    close(fd_sv_fifo);
    unlink(BKND_FIFO); //Fecha canal do servidor
    printf("[INFO] Plataforma terminada!\n");
    return 0;
}