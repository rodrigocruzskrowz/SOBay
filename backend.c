#include "backend.h"

int parar = 0;
int pidf;

void addUserConnection(User ut, User *connUt, int *nusers){
    connUt[*nusers++] = ut;
}

void stopReadPromotor(int sign){
    union sigval val;
    val.sival_int = 1;
    sigqueue(pidf,SIGUSR1,val);
    signal(SIGINT,SIG_DFL);
    parar = 1;
}
void stopValidatingLogs(int sign){
    signal(SIGINT,SIG_DFL);
    parar = 1;
}

void initPlataforma(){
    int fd_init = open(FINIT, O_RDONLY);
    if(fd_init == -1){
        printf("\nFicheiro de inicialização não encontrado, a gerar novo ficheiro de inicialização...\n");
        fd_init = open(FINIT, O_WRONLY | O_CREAT,0600);
        char initFile[MAX_SIZE] = "1 1";
        write(fd_init,initFile,strlen(initFile));
        printf("Ficheiro de inicialização gerado!\n\n");
    }
    else{
        char c[MAX_SIZE];
        int tempo, proxID;
        int nbytes;
        while((nbytes = read(fd_init,&c,sizeof(c)-1)) > 0){
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

//Lê ficheiro de items
//Retorna: 1-Sucesso; 0-Erro;
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
    Tempo *td = data;

    sleep(1);

    while(td->para == 1){

        //Incrementa tempo
        TEMPO++;

        //Decrementa tempo de items em leilao

        //Decrementa tempo de promoções (?)

        //Avisa todos os utilizadores de um acontecimento
        //(Item X comprado; Item Y Acabou o tempo;...)

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
            int res = pthread_mutex_lock(trr->ptrinco);
            if(res != 0){
                printf("[ERROR] %s\n", strerror(errno));
            }

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
            else if(strcmp(comm.word,"SHUTDOWN")==0){
                printf("[INFO] Pedido de encerramento recebido.\n");
            }
        }
        //ESPERAR PELO TRINCO (DESBLOQUEAR)
        pthread_mutex_unlock(trr->ptrinco);
    }
}

int main() {

    initPlataforma();

    User connectedUsers[MAX_USERS];
    int nConnectedUsers = 0;
    Item item_lista[MAX_ITEMS];
    int nitems_lista = 0;
    int fd_sv_fifo;

    //Verifica se backend já está em execução
    if(access(BKND_FIFO,F_OK) == 0){
        printf("O BACKEND já está em execução!\n");
//        exit(1);
    }

    //Verifica se a variavel de ambiente FUSERS existe
    if(getenv("FUSERS") == NULL){
        printf("A variável de ambiente 'FUSERS' não foi definida.\n");
      FUSERS = "../users.txt";
//        exit(1);
    }
    else{
        FUSERS = getenv("FUSERS");
        printf("\nVariável de ambiente 'FUSERS' = %s\n",FUSERS);
    }

    //Verifica se a variavel de ambiente FPROMOTERS existe
    if(getenv("FPROMOTERS") == NULL){
        printf("A variável de ambiente 'FPROMOTERS' não foi definida.\n");
        FPROMOTERS = "../promotores.txt";
//        exit(1);
    }
    else{
        FPROMOTERS = getenv("FPROMOTERS");
        printf("\nVariável de ambiente 'FPROMOTERS' = %s\n",FPROMOTERS);
    }

    //Verifica se a variavel de ambiente FITEMS existe
    if(getenv("FITEMS") == NULL){
        printf("A variável de ambiente 'FITEMS' não foi definida.\n");
        FITEMS = "../items.txt";
//        exit(1);
    }
    else{
        FITEMS = getenv("FITEMS");
        printf("\nVariável de ambiente 'FITEMS' = %s\n",FITEMS);
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
    pthread_mutex_t trinco;
    pthread_mutex_init(&trinco,NULL);
    pthread_t thread[NUM_OF_THREADS];
    RR rrdata;

    rrdata.connUt = connectedUsers;
    rrdata.nConnUt = &nConnectedUsers;
    rrdata.listIt = item_lista;
    rrdata.nlistIt = &nitems_lista;
    rrdata.ptrinco = &trinco;
    rrdata.fd_sv_fifo = fd_sv_fifo;
    rrdata.para = 1;

    int ver = pthread_create(&thread[0],NULL,respondeUsers,&rrdata);
    if(ver){
        printf("[ERROR] Erro ao criar a thread %d (validação de login).\n",ver);
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
                printf("Comando válido!\n");
            } else {
                printf("[WARNING] O comando inserido não é válido.\n");
                printf("Formato esperado:\n\tusers\n\n");
            }
        } else if (strcmp(comando[0], "list") == 0) {
            if (c == 1) {
                //Listar items à venda
                printf("Comando válido!\n");
            } else {
                printf("[WARNING] O comando inserido não é válido.\n");
                printf("Formato esperado:\n\tlist\n\n");
            }
        } else if (strcmp(comando[0], "kick") == 0) {
            if (c == 2) {
                //Verificar se utilizador existe

                //Expulsar o utilizador
                printf("Comando válido!\n");
            } else {
                printf("[WARNING] O comando inserido não é válido.\n");
                printf("Formato esperado:\n\tkick <username>\n\n");
            }
        } else if (strcmp(comando[0], "prom") == 0) {
            if (c == 1) {
                //Listar promotores ativos
                printf("Comando válido!\n");
            } else {
                printf("[WARNING] O comando inserido não é válido.\n");
                printf("Formato esperado:\n\tprom\n\n");
            }
        } else if (strcmp(comando[0], "reprom") == 0) {
            if (c == 1) {
                //Atualizar promotores
                printf("Comando válido!\n");
            } else {
                printf("[WARNING] O comando inserido não é válido.\n");
                printf("Formato esperado:\n\treprom\n\n");
            }
        } else if (strcmp(comando[0], "cancel") == 0) {
            if (c == 2) {
                //Verificar se o executavel existe

                //Cancelar promotor
                printf("Comando válido!\n");
            } else {
                printf("[WARNING] O comando inserido não é válido.\n");
                printf("Formato esperado:\n\tcancel <nome-do-executável-do-promotor>\n\n");
            }
        } else if (strcmp(comando[0], "close") == 0) {
            if (c == 1) {
                //Notifica frontends


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

    int opc;
    int estado;
    do {
        printf("\n  ::::TEMPORARIO::::\n");
        printf("Deseja testar qual opção?\n");
        printf("2-Execução do promotor\n");
        printf("3-Utilizadores\n");
        printf("6-Lista de items\n");
        printf("0-Terminar\n");
        printf("Opção: ");
        scanf("%d", &opc);
        switch (opc) {
            case 2: {


                int fd, cont = 0, arg = 0, nbytes, i = 0, j = 0, ic = 0;
                char str[MAX_SIZE], c;
                char promotorList[MAX_PROMOTORES][MAX_SIZE];

                fd = open(FPROMOTERS, O_RDONLY);
                if(fd==-1){
                    printf("\nFicheiro de promotores não encontrado!\n");
                    break;
                }
                else {
                    while((nbytes = read(fd,&c,1)) > 0){ //le byte por byte
                        //printf("%c",c);
                        if(c == ' ' || c == '\n'){
                            promotorList[i][j] ='\0';
                            j = 0;

                            if(c == '\n'){
                                if(i < MAX_PROMOTORES)
                                    i++;
                            }

                            memset(str,0,MAX_SIZE);
                            cont = 0;
                        }
                        else{
                            promotorList[i][j] = c;
                            j++;
                        }
                    }
                }

                int res;
                int canal[2];
                pipe(canal);

                res = fork();
                if (res == 0) {
                    close(1);
                    dup(canal[1]);
                    close(canal[1]);
                    close(canal[0]);
                    char o[MAX_SIZE];
                    if(access(promotorList[0],F_OK)==0){
                        execl(promotorList[0], promotorList[0], NULL);
                    }

                    printf("[ERRO] Não foi possível executar o promotor. Verifique se o nome do ficheiro está correto. '%s'\n",promotorList[0]);
                    exit(1);
                }

                close(canal[1]);
                pidf = res;
                parar = 0;
                struct sigaction sa;
                sa.sa_handler = stopReadPromotor;
                sa.sa_flags = SA_SIGINFO;
                sigaction(SIGINT,&sa,NULL);

                promocao promo[MAX_SIZE];
                i=0;
                while (parar == 0) {
                    res = read(canal[0], str, sizeof(str)-1);
                    if (res > 0) {
                        str[res] = '\0';
                        printf("O promotor enviou-me: %s", str);

                        char s[MAX_SIZE];
                        cont = 0;
                        for(int itr=0; itr<strlen(str);itr++){
                            if(str[itr] != '\n' && str[itr] != ' '){
                                s[cont++] = str[itr];
                            }
                            else{
                                s[cont] = '\0';
                                cont = 0;
                                switch (arg) {
                                    case 0:{
                                        strcpy(promo[i].categoria,s);
                                        memset(s,0,MAX_SIZE);
                                        break;
                                    }
                                    case 1:{
                                        promo[i].promo = atoi(s);
                                        memset(s,0,MAX_SIZE);
                                        break;
                                    }
                                    case 2:{
                                        promo[i].tempo = atoi(s);
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
                                    printf("\n:::PROMOÇÃO %d:::\n",i+1);
                                    printf("Categoria: %s\n", promo[i].categoria);
                                    printf("Promoção: %d\n", promo[i].promo);
                                    printf("Tempo: %d\n\n", promo[i].tempo);
                                    i++;
                                }
                            }
                        }
                        wait(&estado);
                    }
                }
                break;
            }
            case 3: {

                int nUtilizadores = loadUsersFile(FUSERS);
                printf("\nNum. Utilizadores no ficheiro: %d\n",nUtilizadores);

                char usr[MAX_SIZE];
                int saldo = 0;

                printf("\nUtilizador a procurar: ");
                scanf("%s",usr);

                saldo = getUserBalance(usr);
                printf("\nSaldo atual: %d\n",saldo);
                if(saldo == -1){
                    printf("[ERRO] Não foi possível obter o saldo.\n%s\n", getLastErrorText());
                    break;
                }

                saldo -= 1;
                if(updateUserBalance(usr, saldo) == 1){
                    printf("Saldo do utilizador %s atualizado!\n",usr);
                    printf("Saldo após taxa de teste (1): %d",getUserBalance(usr));
                }
                else{
                    printf("Erro ao atualizar o saldo do utilizador %s.\n%s\n",usr ,getLastErrorText());
                }

                saveUsersFile(FUSERS);

                break;
            }
            case 6:{
                for(int i=0;i<nitems_lista;i++)
                {
                    printf("\n:::ITEM %d:::\n",i+1);
                    printf("ID: %d\n", item_lista[i].id);
                    printf("Item: %s\n", item_lista[i].nome);
                    printf("Categoria: %s\n", item_lista[i].categoria);
                    printf("Licitação: %d\n", item_lista[i].bid);
                    printf("Compre já: %d\n", item_lista[i].buyNow);
                    printf("Tempo de venda: %d\n", item_lista[i].tempo);
                    printf("Vendedor: %s\n", item_lista[i].vendedor);
                    printf("Licitador: %s\n", item_lista[i].licitador);
                }
            }
            case 0:{

                break;
            }
            default:
                printf("Opção desconhecida.");
                break;
        }
        printf("\n\n\n");
    }while(opc!=0);

    for(int i=0; i<nConnectedUsers; i++){
        printf("%s %d\n",connectedUsers[i].nome,connectedUsers[i].saldo);
    }

    for(int i=0;i<NUM_OF_THREADS;i++){
        rrdata.para = 0;
        CA end;
        strcpy(end.word,"SHUTDOWN");
        int n = write(fd_sv_fifo,&end,sizeof(CA));
        if(n == sizeof(User)){
            printf("[INFO] Enviei %s\n",end.word);
        }
        pthread_join(thread[i],NULL);
    }
    pthread_mutex_destroy(&trinco);

    close(fd_sv_fifo);
    unlink(BKND_FIFO); //Fecha canal do servidor
    printf("[INFO] Plataforma terminada!\n");
    return 0;
}