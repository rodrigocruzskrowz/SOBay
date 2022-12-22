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

                    nitems_lista++;
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
    Item it;

    //Atende pedidos
    while(trr->para == 1){
        int lido = read(trr->fd_sv_fifo,&comm,sizeof(CA));
        if(lido == sizeof(CA)){
            if(strcmp(comm.word,"CRIAR")==0){
                //Lê dados do item
                Item it;
                int lido = read(trr->fd_sv_fifo,&it,sizeof(Item));
                if(lido == sizeof(Item)){
                    printf("\nItem recebido:\nId: %d\nNome: %s\nCategoria: %s\nPreço atual: %d\nPreço compre já: %d\nTempo até fim de leilão: %d\nVendedor: %s, Licitador: %s\n",
                           it.id,it.nome,it.categoria,it.bid,it.buyNow,it.tempo,it.vendedor,it.licitador);

                    //Coloca item à venda (adiciona à lista de items)
                    trr->listIt[*trr->nlistIt].id = PROX_ID;
                    strcpy(trr->listIt[*trr->nlistIt].nome, it.nome);
                    strcpy(trr->listIt[*trr->nlistIt].categoria, it.categoria);
                    trr->listIt[*trr->nlistIt].bid = it.bid;
                    trr->listIt[*trr->nlistIt].buyNow = it.buyNow;
                    trr->listIt[*trr->nlistIt].tempo = it.tempo;
                    strcpy(trr->listIt[*trr->nlistIt].vendedor, it.vendedor);
                    strcpy(trr->listIt[*trr->nlistIt].licitador, it.licitador);
                    PROX_ID++;
                    *trr->nlistIt++;

                    CA comm;
                    strcpy(comm.word,"INSERIDO");
                    comm.number = *trr->nlistIt;
                    comm.secNumber = trr->listIt[*trr->nlistIt-1].id;
                    write(trr->fd_sv_fifo,&comm,sizeof(CA));
                }

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

            if(strcmp(comm.word,"LISTAR")==0){
                //Enviar sucesso na receção do comando
                strcpy(comm.word,"ENVIADO");
                comm.number = *trr->nlistIt;
                write(trr->fd_sv_fifo,&comm,sizeof(CA));

                //Enviar lista de items
                int itemsEnviados = 0;
                for(int i=0; i<*trr->nlistIt; i++){
                    int n = write(trr->fd_sv_fifo,&trr->listIt[i],sizeof(Item));
                    if(n == sizeof(Item)){
                        itemsEnviados++;
                        //printf("[INFO] Enviei %d %s %s %d %d %d %s %s\n\n",it.id,it.nome,it.categoria,it.bid,it.buyNow,it.tempo,it.vendedor,it.licitador);
                    }
                }
                printf("Enviei %d items.\n",itemsEnviados);
            }

            if(strcmp(comm.word,"LISTCAT")==0){
                //Verifica se existem items com a categoria recebida
                Item it_cat_list[MAX_ITEMS];
                int itemsAEnviar = 0;
                for(int i=0;i<*trr->nlistIt;i++){
                    if(strcmp(trr->listIt[i].categoria,comm.secWord)==0){
                        it_cat_list[itemsAEnviar++] = trr->listIt[i];
                    }
                }

                if(itemsAEnviar > 0){
                    //Enviar sucesso na receção do comando
                    strcpy(comm.word,"ENVCAT");
                    comm.number = itemsAEnviar;
                    write(trr->fd_sv_fifo,&comm,sizeof(CA));

                    //Enviar items da categoria recebida
                    for(int i=0;i<itemsAEnviar;i++){
                        write(trr->fd_sv_fifo,&it_cat_list[i],sizeof(Item));
                    }
                    printf("Enviei %d items.\n",itemsAEnviar);
                }
                else{
                    //Enviar erro (nenhum item a listar)
                    strcpy(comm.word,"ENVCAT");
                    comm.number = 0;
                    write(trr->fd_sv_fifo,&comm,sizeof(CA));
                }
            }

            if(strcmp(comm.word,"LISTSEL")==0){
                //Verifica se existem items do vendedor recebida
                Item it_sel_list[MAX_ITEMS];
                int itemsAEnviar = 0;
                for(int i=0;i<*trr->nlistIt;i++){
                    if(strcmp(trr->listIt[i].vendedor,comm.secWord)==0){
                        it_sel_list[itemsAEnviar++] = trr->listIt[i];
                    }
                }

                if(itemsAEnviar > 0){
                    //Enviar sucesso na receção do comando
                    strcpy(comm.word,"ENVSEL");
                    comm.number = itemsAEnviar;
                    write(trr->fd_sv_fifo,&comm,sizeof(CA));

                    //Enviar items da categoria recebida
                    for(int i=0;i<itemsAEnviar;i++){
                        write(trr->fd_sv_fifo,&it_sel_list[i],sizeof(Item));
                    }
                    printf("Enviei %d items.\n",itemsAEnviar);
                }
                else{
                    //Enviar erro (nenhum item a listar)
                    strcpy(comm.word,"ENVSEL");
                    comm.number = 0;
                    write(trr->fd_sv_fifo,&comm,sizeof(CA));
                }
            }

            if(strcmp(comm.word,"LISTVAL")==0){
                //Verifica se existem items ate ao valor recebido
                Item it_val_list[MAX_ITEMS];
                int itemsAEnviar = 0;
                for(int i=0;i<*trr->nlistIt;i++){
                    if(trr->listIt[i].bid <= comm.number){
                        it_val_list[itemsAEnviar++] = trr->listIt[i];
                    }
                }

                if(itemsAEnviar > 0){
                    //Enviar sucesso na receção do comando
                    strcpy(comm.word,"ENVVAL");
                    comm.number = itemsAEnviar;
                    write(trr->fd_sv_fifo,&comm,sizeof(CA));

                    //Enviar items da categoria recebida
                    for(int i=0;i<itemsAEnviar;i++){
                        write(trr->fd_sv_fifo,&it_val_list[i],sizeof(Item));
                    }
                    printf("Enviei %d items.\n",itemsAEnviar);
                }
                else{
                    //Enviar erro (nenhum item a listar)
                    strcpy(comm.word,"ENVVAL");
                    comm.number = 0;
                    write(trr->fd_sv_fifo,&comm,sizeof(CA));
                }
            }

            if(strcmp(comm.word,"LISTIM")==0){
                //Verifica se existem items ate ao valor recebido
                Item it_tim_list[MAX_ITEMS];
                int itemsAEnviar = 0;
                for(int i=0;i<*trr->nlistIt;i++){
                    if((TEMPO + trr->listIt[i].tempo) >= comm.number){
                        it_tim_list[itemsAEnviar++] = trr->listIt[i];
                    }
                }

                if(itemsAEnviar > 0){
                    //Enviar sucesso na receção do comando
                    strcpy(comm.word,"ENVTIM");
                    comm.number = itemsAEnviar;
                    write(trr->fd_sv_fifo,&comm,sizeof(CA));

                    //Enviar items da categoria recebida
                    for(int i=0;i<itemsAEnviar;i++){
                        write(trr->fd_sv_fifo,&it_tim_list[i],sizeof(Item));
                    }
                    printf("Enviei %d items.\n",itemsAEnviar);
                }
                else{
                    //Enviar erro (nenhum item a listar)
                    strcpy(comm.word,"ENVTIM");
                    comm.number = 0;
                    write(trr->fd_sv_fifo,&comm,sizeof(CA));
                }
            }

            if(strcmp(comm.word,"TIME")==0){
                //Enviar tempo atual
                strcpy(comm.word,"ENVTIME");
                comm.number = TEMPO;
                write(trr->fd_sv_fifo,&comm,sizeof(CA));
            }

            if(strcmp(comm.word,"BUY")==0){
                //word-IDENTIFICATION WORD; secWord-USERNAME; number-ITEM ID; secNumber-VALOR
                int encontrou = 0;
                int compra = 0;
                int novoSaldo;

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
                                write(trr->fd_sv_fifo,&comm,sizeof(CA));
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
                                write(trr->fd_sv_fifo,&comm,sizeof(CA));
                            }
                            else{
                                //Saldo insuficiente
                                strcpy(comm.word,"ERRSALDO");
                                comm.number = saldo;
                                write(trr->fd_sv_fifo,&comm,sizeof(CA));
                            }
                        }
                        else{
                            strcpy(comm.word,"ERRVAL");
                            write(trr->fd_sv_fifo,&comm,sizeof(CA));
                        }
                    }
                }

                if(compra == 1){
                    *trr->nlistIt--;

                    //Envia para frontend
                    strcpy(comm.word,"BOUGHT");
                    comm.number = novoSaldo;
                    write(trr->fd_sv_fifo,&comm,sizeof(CA));
                }

                if(encontrou == 0){
                    //Envia para frontend
                    strcpy(comm.word,"ERRID");
                    write(trr->fd_sv_fifo,&comm,sizeof(CA));
                }
            }

            if(strcmp(comm.word,"CASH")==0){
                //Enviar saldo
                comm.number = getUserBalance(comm.secWord);
                strcpy(comm.word,"ENVCASH");
                write(trr->fd_sv_fifo,&comm,sizeof(CA));
            }

            if(strcmp(comm.word,"ADDMONEY")==0){
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
                write(trr->fd_sv_fifo,&comm,sizeof(CA));
            }
        }
    }
}

void *validaLogin(void *data){
    LogData *ld = data;
    int fd_cli_fifo;
    char res_cli_fifo[MAX_SIZE_FIFO];

    //Para parar a thread é preciso mandar tambem qualquer coisa para ser lida de forma a
    //desbloquear o READ.
    while(ld->para == 1) {
        int dados = read(ld->fd_sv_fifo, &ld->ut, sizeof(User));
        if (dados == sizeof(User)) {
            printf("\n\n[INFO] Logs recebidos: %s %s\n", ld->ut.nome, ld->ut.password);
        }

        //Verifica
        if(strcmp(ld->ut.nome, "terminarProgReq") == 0 && strcmp(ld->ut.password, "terminarProgReq") == 0){
            printf("[INFO] A terminar thread\n");
            continue;
        }
        int encontrou = 0;
        for (int i = 0; i < *ld->nConnUt; i++) {
            if (ld->connUt->pid == ld->ut.pid) {
                encontrou = 1;
                printf("[ERROR] O utilizador %s já está na tabela de utilizadores conectados.\n", ld->ut.nome);
            }
        }

        if (encontrou == 0) {
            //ESPERAR PELO TRINCO (BLOQUEAR)
            pthread_mutex_lock(ld->ptrinco);

            ld->ut.valid = isUserValid(ld->ut.nome, ld->ut.password);
            if (ld->ut.valid == 0) {
                sprintf(res_cli_fifo, FRND_FIFO, ld->ut.pid);
                fd_cli_fifo = open(res_cli_fifo, O_WRONLY);
                write(fd_cli_fifo, &ld->ut, sizeof(User));
                printf("[INFO] Tentativa de acesso com credenciais erradas.\n");
            } else if (ld->ut.valid == 1) {
                ld->ut.saldo = getUserBalance(ld->ut.nome);
                sprintf(res_cli_fifo, FRND_FIFO, ld->ut.pid);
                fd_cli_fifo = open(res_cli_fifo, O_WRONLY);
                write(fd_cli_fifo, &ld->ut, sizeof(User));
                printf("[INFO] O utilizador '%s' fez login na plataforma.\n", ld->ut.nome);
                addUserConnection(ld->ut, ld->connUt, ld->nConnUt);
            } else {
                printf("[ERRO] %s\n", getLastErrorText());
            }

            //ESPERAR PELO TRINCO (DESBLOQUEAR)
            pthread_mutex_unlock(ld->ptrinco);
        }
    }

    pthread_exit(NULL);
}

int main() {

    initPlataforma();

    User connectedUsers[MAX_USERS];
    int nConnectedUsers = 0;
    Item item_lista[MAX_ITEMS];
    int nitems_lista = 0;
    int fd_sv_fifo;
    int fd_cli_fifo;
    char res_cli_fifo[MAX_SIZE_FIFO];

    //Verifica se backend já está em execução
    if(access(BKND_FIFO,F_OK) == 0){
        printf("O BACKEND já está em execução!\n");
        exit(1);
    }

    //Verifica se a variavel de ambiente FUSERS existe
    if(getenv("FUSERS") == NULL){
        printf("A variável de ambiente 'FUSERS' não foi definida.\n");
//      FUSERS = "../users.txt";
        exit(1);
    }
    else{
        FUSERS = getenv("FUSERS");
        printf("\nVariável de ambiente 'FUSERS' = %s\n",FUSERS);
    }

    //Verifica se a variavel de ambiente FPROMOTERS existe
    if(getenv("FPROMOTERS") == NULL){
        printf("A variável de ambiente 'FPROMOTERS' não foi definida.\n");
        exit(1);
    }
    else{
        FPROMOTERS = getenv("FPROMOTERS");
        printf("\nVariável de ambiente 'FPROMOTERS' = %s\n",FPROMOTERS);
    }

    //Verifica se a variavel de ambiente FITEMS existe
    if(getenv("FITEMS") == NULL){
        printf("A variável de ambiente 'FITEMS' não foi definida.\n");
        exit(1);
    }
    else{
        FITEMS = getenv("FITEMS");
        printf("\nVariável de ambiente 'FITEMS' = %s\n",FITEMS);
    }

    //Acede ao ficheiro de utilizadores
    int nUtilizadores = loadUsersFile(FUSERS);
    printf("\nNum. Utilizadores no ficheiro: %d\n",nUtilizadores);

    int itemSuccess = readItemsFile(item_lista,&nitems_lista);
    if(itemSuccess == 0){
        exit(1);
    }

    //Cria BACKEND FIFO
    mkfifo(BKND_FIFO,0600);
    fd_sv_fifo = open(BKND_FIFO,O_RDWR);

    //Cria mutex - thread
    pthread_mutex_t trinco;
    pthread_mutex_init(&trinco,NULL);
    pthread_t thread[NUM_OF_THREADS];
    LogData tlogdata;

    //Inicializa estrutura a passar para a thread de validação de login
    tlogdata.connUt = connectedUsers;
    tlogdata.nConnUt = &nConnectedUsers;
    tlogdata.fd_sv_fifo = fd_sv_fifo;
    tlogdata.ptrinco = &trinco;
    tlogdata.para = 1;

    int ver = pthread_create(&thread[0],NULL,validaLogin,&tlogdata);
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
        printf("\nDeseja testar qual opção?\n");
        printf("1-Comandos\n");
        printf("2-Execução do promotor\n");
        printf("3-Utilizadores\n");
        printf("4-Items\n");
        printf("5-Responde a comandos\n");
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
            case 4: {
                //Verifica se a variavel de ambiente FITEMS existe
                if(getenv("FITEMS") == NULL){
                    printf("A variável de ambiente 'FITEMS' não foi definida.\n");
                    //FITEMS = "../items.txt";
                    exit(1);
                }
                else{
                    FITEMS = getenv("FITEMS");
                    printf("\nVariável de ambiente 'FITEMS' = %s\n\n",FITEMS);
                }


                break;
            }
            case 5:{

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
        tlogdata.para = 0;
        User us;
        strcpy(us.nome,"terminarProgReq");
        strcpy(us.password,"terminarProgReq");
        int n = write(fd_sv_fifo,&us,sizeof(User));
        if(n == sizeof(User)){
            printf("[INFO] Enviei %s %s\n",us.nome, us.password);
        }
        pthread_join(thread[i],NULL);
    }
    pthread_mutex_destroy(&trinco);

    close(fd_cli_fifo);
    close(fd_sv_fifo);
    unlink(res_cli_fifo); //Fecha canal do cliente
    unlink(BKND_FIFO); //Fecha canal do servidor
    printf("\n[INFO] Plataforma terminada! Todos os canais foram fechados.\n");
    return 0;
}