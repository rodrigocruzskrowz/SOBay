#include "frontend.h"

int fd_frtnd, fd_bknd, fd_hb;
char *fifo;

void *sendHeartBeat(void *pdata){
    HB *thb = pdata;
    CA comm;

    while(thb->para == 1){
        //ESPERAR PELO TRINCO (BLOQUEAR)
        pthread_mutex_lock(thb->ptrinco);

        strcpy(comm.secWord,"HEARTBEAT");
        strcpy(comm.word,thb->nomeUser);
        comm.ut.pid = thb->pid;
        //int fdhb = open(HB_FIFO,O_WRONLY);
        int n = write(thb->fd_hb_fifo, &comm, sizeof(CA));
//        if(n == sizeof(CA)){
//            printf("\n[INFO] Enviei %s\n", comm.secWord);
//        }
        //close(fdhb);

        //ESPERAR PELO TRINCO (DESBLOQUEAR)
        pthread_mutex_unlock(thb->ptrinco);

        sleep(thb->heartbeat);
    }

    pthread_exit(NULL);
}

void sendHeartbeat(){
    CA comm;
    strcpy(comm.word,"HEARTBEAT");
    comm.ut.pid = getpid();
    int fdhb = open(HB_FIFO,O_WRONLY);
    int n = write(fdhb, &comm, sizeof(CA));
//    if(n == sizeof(CA)){
//        printf("[INFO] Enviei %s\n", comm.word);
//    }
    close(fdhb);
}

void encerraFrontend(int sign){
    close(fd_frtnd);
    close(fd_bknd);
    close(fd_hb);
    unlink(fifo);
    printf("\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    int fd_bknd_fifo;
    int fd_cli_fifo;
    char cli_fifo[MAX_SIZE_FIFO];
    pthread_mutex_t trinco;
    pthread_mutex_init(&trinco,NULL);
    pthread_t threadHB;
    HB hbdata;

    if(argc == 3){
        CA main;
        int HEARTBEAT;

        strcpy(main.word, "LOGIN");
        strcpy(main.ut.nome, argv[1]);
        strcpy(main.ut.password, argv[2]);
        main.ut.saldo = -1;
        main.ut.valid = 0;
        main.ut.pid = getpid();

        //Verifica se o backend está em execução
        if(access(BKND_FIFO, F_OK) != 0){
            printf("[ERROR] O BACKEND não está em execução.\n");
            exit(1);
        }

        fd_bknd_fifo = open(BKND_FIFO,O_WRONLY);
        if(fd_bknd_fifo == -1){
            printf("[ERROR] Não foi possível abrir o canal de comunicação com o BACKEND.\n");
            exit(1);
        }

        int fd_hb_fifo = open(HB_FIFO, O_WRONLY);
        if(fd_hb_fifo == -1){
            printf("[ERROR] Não foi possível abrir o canal de comunicação com o BACKEND (HEARTBEATS).\n");
            exit(1);
        }

        //Cria FIFO do cliente
        sprintf(cli_fifo, FRND_FIFO, main.ut.pid);
        if(access(cli_fifo,F_OK) != 0)
            mkfifo(cli_fifo,0600);
        fd_cli_fifo = open(cli_fifo,O_RDWR);
        if(fd_cli_fifo == -1){
            printf("[ERROR] Não foi possível criar o canal de comunicação com o BACKEND.\n");
            exit(1);
        }

        fd_frtnd = fd_cli_fifo;
        fd_bknd = fd_bknd_fifo;
        fd_hb = fd_hb_fifo;
        fifo = cli_fifo;

        //Modifica comportamento do CTRL + C
        struct sigaction sa;
        sa.sa_handler = encerraFrontend;
        sa.sa_flags = SA_SIGINFO;
        sigaction(SIGINT,&sa,NULL);

        //Envia login para o BACKEND
        int n = write(fd_bknd_fifo, &main, sizeof(CA));
//        if(n == sizeof(CA)){
//            printf("[INFO] Enviei %s %s %d\n", main.ut.nome, main.ut.password, main.ut.pid);
//            printf("[INFO] Pedi para autenticar\n\n");
//        }
        //Recebe validação do login
        int resposta;
        resposta = read(fd_cli_fifo, &main, sizeof(CA));
        if(resposta == sizeof(CA)){
            if(main.ut.valid == 1){
                HEARTBEAT = main.number;
//                printf("HEARTBEAT: %d",main.number);
                printf("Olá, %s!\nO seu saldo é de %d SOCoins.\n\n", main.ut.nome, main.ut.saldo);
            }
            else{
                printf("[INFO] Credênciais erradas. Acesso negado!\n\n");
                close(fd_bknd_fifo);
                close(fd_cli_fifo);
                unlink(cli_fifo);
                exit(1);
            }
        }

        //Inicia thread hearthbeat
        hbdata.pid = main.ut.pid;
        strcpy(hbdata.nomeUser,main.ut.nome);
        hbdata.fd_hb_fifo = fd_hb_fifo;
        hbdata.heartbeat = HEARTBEAT;
        hbdata.ptrinco = &trinco;
        hbdata.para = 1;

        int ver = pthread_create(&threadHB, NULL, sendHeartBeat, &hbdata);
        if(ver != 0){
            printf("[ERROR] Erro ao criar a thread %d (envio de heartbeat).\n",ver);
            close(fd_cli_fifo);
            close(fd_bknd_fifo);
            close(fd_hb_fifo);
            exit(1);
        }

        //Select
        fd_set fd;
        int res_sel;
        //struct timeval timeout;

        int printComando = 1;
        while(1){
            if(printComando == 1) {
                printf("\ncomando > ");
                fflush(stdout);
                printComando = 0;
            }

            //HEARTBEAT
//            struct sigaction sa;
//            sa.sa_sigaction = sendHeartbeat;
//            sa.sa_flags = SA_SIGINFO;
//            sigaction(SIGALRM,&sa,NULL);
//            alarm(HEARTBEAT);

            FD_ZERO(&fd);
            FD_SET(0,&fd);
            FD_SET(fd_cli_fifo, &fd);
//            timeout.tv_sec = HEARTBEAT;
//            timeout.tv_usec = 0;

            res_sel = select(fd_cli_fifo + 1, &fd, NULL, NULL, NULL);
//            res_sel = select(fd_cli_fifo + 1, &fd, NULL, NULL, &timeout);

            if(res_sel == -1){
                //ERRO
                printf("[ERROR] ERRO NO SELECT - %s\n", strerror(errno));
                break;
            }
//            if(res_sel == 0){
//                //Timeout
//
//            }
            if(res_sel > 0 && FD_ISSET(fd_cli_fifo, &fd)){
                //Trata dados recebidos
                CA comm;
                int resposta = read(fd_cli_fifo,&comm,sizeof(CA));
                if(resposta == sizeof(CA)){
                    if(strcmp(comm.word,"SHUTDOWNALL") == 0){
                        printf("[INFO] O BACKEND informou que vai encerrar. A terminar plataforma...\n\n");
                        //Termina cliente
                        break;
                    }

                    else if(strcmp(comm.word,"ITEXPIRED") == 0){
                        printf("\n[INFO] O item '%s' expirou!\n\n",comm.secWord);
                    }

                    else if(strcmp(comm.word,"ITBOUGHT") == 0){
                        printf("\n[INFO] O item '%s' foi comprado!\n\n",comm.secWord);
                    }

                    else if(strcmp(comm.word,"ITLICIT") == 0){
                        printf("\n[INFO] O item '%s' foi licitado!\n\n",comm.secWord);
                    }

                    else if(strcmp(comm.word,"UTKICK") == 0){
                        printf("\n[INFO] Foi expulso da plataforma pelo administrador.\n\n");
                        //Termina cliente
                        break;
                    }

                    else if(strcmp(comm.word,"INSERIDO") == 0){
                        printf("[INFO] O artigo foi adicionado ao leilão com o id %d!\n\n",comm.secNumber);
                    }

                    else if(strcmp(comm.word,"ENVIADO")==0){
                        //Recebe lista de items
                        printf("\n\n:::::LISTA DE ITEMS:::::");
                        imprimeItems(comm.it,comm.number);
                    }

                    else if(strcmp(comm.word,"ENVCAT")==0){
                        if(comm.number <= 0){
                            printf("[INFO] Não existem items da categoria '%s' em leilão.\n",comm.secWord);
                        }
                        else{
                            //Recebe lista de items
                            printf("\n\n:::::LISTA DE ITEMS:::::");
                            imprimeItems(comm.it,comm.number);
                        }
                    }

                    else if(strcmp(comm.word,"ENVSEL")==0){
                        if(comm.number <= 0){
                            printf("[INFO] Não existem items do vendedor '%s' em leilão.\n",comm.secWord);
                        }
                        else{
                            //Recebe lista de items
                            printf("\n\n:::::LISTA DE ITEMS:::::");
                            imprimeItems(comm.it,comm.number);
                        }
                    }

                    else if(strcmp(comm.word,"ENVVAL")==0){
                        if(comm.number <= 0){
                            printf("[INFO] Não existem items com valor até %d.\n",comm.number);
                        }
                        else{
                            //Recebe lista de items
                            printf("\n\n:::::LISTA DE ITEMS:::::");
                            imprimeItems(comm.it,comm.number);
                        }
                    }

                    else if(strcmp(comm.word,"ENVTIM")==0){
                        if(comm.number <= 0){
                            printf("[INFO] Não existem items em leilão à hora %d.\n",comm.number);
                        }
                        else{
                            //Recebe lista de items
                            printf("\n\n:::::LISTA DE ITEMS:::::");
                            imprimeItems(comm.it,comm.number);
                        }
                    }

                    else if(strcmp(comm.word,"ENVTIME")==0){
                        printf("[INFO] Hora atual: %d\n",comm.number);
                    }

                    else if(strcmp(comm.word,"ERRSALDO")==0){
                        printf("[INFO] O seu saldo não é suficiente para completar a transação.\nO seu saldo é de %d SOCoins.",comm.ut.saldo);
                    }
                    else if(strcmp(comm.word,"BOUGHT")==0){
                        printf("[INFO] O item %d foi comprado com sucesso!\nO seu saldo atual é de %d SOCoins.\n",comm.number,comm.ut.saldo);
                    }
                    else if(strcmp(comm.word,"BIDDED")==0){
                        printf("[INFO] Licitação de %d colocada no item %d.\nO seu saldo atual é de %d SOCoins.",comm.secNumber,comm.number,comm.ut.saldo);
                    }
                    else if(strcmp(comm.word,"ERRID")==0){
                        printf("[INFO] Item com o id %d não encontrado.\n",comm.number);
                    }
                    else if(strcmp(comm.word,"ERRVAL")==0){
                        printf("[INFO] O valor de licitação é inferior ao preço atual do item, por favor faça nova licitação.\n");
                    }

                    else if(strcmp(comm.word,"ENVCASH")==0){
                        printf("[INFO] O meu saldo atual: %d SOCoins\n",comm.number);
                    }

                    else if(strcmp(comm.word,"ENVADDMONEY")==0){
                        printf("[INFO] Foram adicionadas %d SOCoins ao seu saldo.\nO seu saldo atual é de %d SOCoins.",comm.number,comm.ut.saldo);
                    }
                }
            }
            if(res_sel > 0 && FD_ISSET(0, &fd)){
                //Lê comando
                printComando = 1;
                char str[MAX_SIZE];
                char comando[6][MAX_SIZE];
                int i=0,j=0,c=0;
                fgets(str,sizeof str,stdin);
                for(i=0;i<=(strlen(str));i++){
                    if(str[i]==' ' || str[i]=='\n'){
                        comando[c][j]='\0';
                        c++;
                        j=0;
                    }
                    else{
                        comando[c][j]=str[i];
                        j++;
                    }
                }

    //        for(int i=0;i<=c;i++){
    //            printf("%s\n",comando[i]);
    //        }

                if(strcmp(comando[0],"sell")==0){
                    if(c==6) {
                        CA comm;
                        int isValid = 1;

                        //Verificar se preço é inteiro
                        j=0;
                        while(comando[3][j]!='\0'){
                            if (isdigit(comando[3][j]) == 0) {
                                printf("[WARNING] O parâmetro recebido para o 'preço base' não é um inteiro.");
                                isValid = 0;
                                break;
                            }

                            j++;
                        }

                        if(isValid == 0)
                            continue;

                        //Verificar se buyNow é inteiro
                        j=0;
                        while(comando[4][j]!='\0'){
                            if (isdigit(comando[4][j]) == 0) {
                                printf("[WARNING] O parâmetro recebido para o 'preço compre já' não é um inteiro.");
                                isValid = 0;
                                break;
                            }

                            j++;
                        }

                        if(isValid == 0)
                            continue;

                        //Verificar se duração é inteiro
                        j=0;
                        while(comando[5][j]!='\0'){
                            if (isdigit(comando[5][j]) == 0) {
                                printf("[WARNING] O parâmetro recebido para a 'duração' não é um inteiro.");
                                isValid = 0;
                                break;
                            }

                            j++;
                        }

                        if(isValid == 0)
                            continue;

//                        printf("Comando válido!\n");

                        //Informa o backend de que quer criar item
                        comm.ut.pid = main.ut.pid;
                        strcpy(comm.word,"CRIAR");
                        strcpy(comm.it[0].nome,comando[1]);
                        strcpy(comm.it[0].categoria,comando[2]);
                        comm.it[0].bid = atoi(comando[3]);
                        comm.it[0].buyNow = atoi(comando[4]);
                        comm.it[0].tempo = atoi(comando[5]);
                        comm.it[0].id = 0;
                        strcpy(comm.it[0].vendedor,main.ut.nome);
                        strcpy(comm.it[0].licitador,"-");
                        int n = write(fd_bknd_fifo,&comm,sizeof(CA));
//                        if(n == sizeof(CA)){
//                            printf("[INFO] Enviei %s %d %s %s %d %d %d %s %s\n\n",comm.word,comm.it[0].id,comm.it[0].nome,comm.it[0].categoria,comm.it[0].bid,comm.it[0].buyNow,comm.it[0].tempo,comm.it[0].vendedor,comm.it[0].licitador);
//                        }
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tsell <nome-Item> <categoria> <preço-base> <preço-compre-já> <duração>\n\n");
                    }
                }

                else if(strcmp(comando[0],"list")==0){
                    if(c==1) {
                        CA comm;
                        //Listar items
//                        printf("Comando válido!\n");

                        //Envia pedido para o backend
                        strcpy(comm.word,"LISTAR");
                        comm.ut.pid = main.ut.pid;
                        int n = write(fd_bknd_fifo,&comm,sizeof(CA));
//                        if(n == sizeof(CA)){
//                            printf("[INFO] Pedi para '%s' items.\n\n",comm.word);
//                        }
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tlist\n\n");
                    }
                }

                else if(strcmp(comando[0],"licat")==0){
                    if(c==2) {
                        CA comm;
                        comm.ut.pid = main.ut.pid;
                        //Listar Items da categoria
//                        printf("Comando válido!\n");

                        //Envia pedido para o backend
                        strcpy(comm.word,"LISTCAT");
                        strcpy(comm.secWord,comando[1]);
                        int n = write(fd_bknd_fifo,&comm,sizeof(CA));
//                        if(n == sizeof(CA)){
//                            printf("[INFO] Pedi os items da categoria: '%s'\n\n",comm.secWord);
//                        }
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tlicat <nome-categoria>\n\n");
                    }
                }

                else if(strcmp(comando[0],"lisel")==0){
                    if(c==2) {
                        CA comm;
                        comm.ut.pid = main.ut.pid;
                        Item it;
                        //Listar Items do vendedor
//                        printf("Comando válido!\n");

                        //Envia pedido para o backend
                        strcpy(comm.word,"LISTSEL");
                        strcpy(comm.secWord,comando[1]);
                        int n = write(fd_bknd_fifo,&comm,sizeof(CA));
//                        if(n == sizeof(CA)){
//                            printf("[INFO] Pedi os items do vendedor: '%s'\n\n",comm.secWord);
//                        }
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tlisel <nome-vendedor>\n\n");
                    }
                }

                else if(strcmp(comando[0],"lival")==0){
                    if(c==2) {
                        CA comm;
                        comm.ut.pid = main.ut.pid;
                        int isValid = 1;
                        //Verificar se preço é inteiro
                        j=0;
                        while(comando[1][j]!='\0'){
                            if (isdigit(comando[1][j]) == 0) {
                                printf("[WARNING] O parâmetro recebido para o 'preço máximo' não é um inteiro.");
                                isValid = 0;
                                break;
                            }

                            j++;
                        }

                        if(isValid == 0)
                            continue;

                        //Listar items até ao preço indicado
//                        printf("Comando válido!\n");
                        //Envia pedido para o backend
                        strcpy(comm.word,"LISTVAL");
                        comm.number = atoi(comando[1]);
                        int n = write(fd_bknd_fifo,&comm,sizeof(CA));
//                        if(n == sizeof(CA)){
//                            printf("[INFO] Pedi os items até %d SOCoins.\n\n",atoi(comando[1]));
//                        }
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tlival <preço-máximo>\n\n");
                    }
                }

                else if(strcmp(comando[0],"litime")==0){
                    if(c==2) {
                        CA comm;
                        comm.ut.pid = main.ut.pid;
                        int isValid = 1;
                        //Verificar se tempo é inteiro
                        j=0;
                        while(comando[1][j]!='\0'){
                            if (isdigit(comando[1][j]) == 0) {
                                printf("[WARNING] O parâmetro recebido para a 'hora em segundos' não é um inteiro.");
                                isValid = 0;
                                break;
                            }

                            j++;
                        }

                        if(isValid == 0)
                            continue;

                        //Listar items até ao preço indicado
//                        printf("Comando válido!\n");
                        //Envia pedido para o backend
                        strcpy(comm.word,"LISTIM");
                        comm.number = atoi(comando[1]);
                        int n = write(fd_bknd_fifo,&comm,sizeof(CA));
//                        if(n == sizeof(CA)){
//                            printf("[INFO] Pedi os items até à hora %d\n\n",atoi(comando[1]));
//                        }
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tlitime <hora-em-segundos>\n\n");
                    }
                }

                else if(strcmp(comando[0],"time")==0){
                    if(c==1) {
                        CA comm;
                        comm.ut.pid = main.ut.pid;
                        //Indicar hora atual
//                        printf("Comando válido!\n");
                        //Envia pedido para o backend
                        strcpy(comm.word,"TIME");
                        int n = write(fd_bknd_fifo,&comm,sizeof(CA));
//                        if(n == sizeof(CA)){
//                            printf("[INFO] Pedi a hora atual\n\n");
//                        }
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\ttime\n\n");
                    }
                }

                else if(strcmp(comando[0],"buy")==0){
                    if(c==3) {
                        int isValid = 1;
                        //Verificar se id é inteiro e existe
                        j=0;
                        while(comando[1][j]!='\0'){
                            if (isdigit(comando[1][j]) == 0) {
                                printf("[WARNING] O parâmetro recebido para o 'id' não é um inteiro.");
                                isValid = 0;
                                break;
                            }

                            j++;
                        }

                        if(isValid == 0)
                            continue;

                        //Verificar se valor é inteiro
                        j=0;
                        while(comando[2][j]!='\0'){
                            if (isdigit(comando[2][j]) == 0) {
                                printf("[WARNING] O parâmetro recebido para o 'valor' não é um inteiro.");
                                isValid = 0;
                                break;
                            }

                            j++;
                        }

                        if(isValid == 0)
                            continue;

                        //Licitar Item
//                        printf("Comando válido!\n");
                        //Envia para o backend
                        CA comm;
                        //word-IDENTIFICATION WORD; secWord-USERNAME; number-ITEM ID; secNumber-VALOR
                        comm.ut.pid = main.ut.pid;
                        strcpy(comm.word,"BUY");
                        strcpy(comm.secWord,main.ut.nome);
                        comm.number = atoi(comando[1]);
                        comm.secNumber = atoi(comando[2]);
                        int n = write(fd_bknd_fifo,&comm,sizeof(CA));
                        if(n == sizeof(CA)){
                            printf("[INFO] Licitei o item %d por %d SOCoins.\n\n",comm.number, comm.secNumber);
                        }
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tbuy <id> <valor>\n\n");
                    }
                }

                else if(strcmp(comando[0],"cash")==0){
                    if(c==1) {
                        CA comm;
                        comm.ut.pid = main.ut.pid;
                        //Mostra saldo
//                        printf("Comando válido!\n");
                        //Envia pedido para o backend
                        strcpy(comm.word,"CASH");
                        strcpy(comm.secWord,main.ut.nome);
                        int n = write(fd_bknd_fifo,&comm,sizeof(CA));
//                        if(n == sizeof(CA)){
//                            printf("[INFO] Pedi para consultar o meu saldo.\n\n");
//                        }
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tcash\n\n");
                    }
                }

                else if(strcmp(comando[0],"add")==0){
                    if(c==2) {
                        int isValid = 1;

                        //Verifica se valor é inteiro
                        j=0;
                        while(comando[1][j]!='\0'){
                            if (isdigit(comando[1][j]) == 0) {
                                printf("[WARNING] O parâmetro recebido para o 'valor' não é um inteiro.");
                                isValid = 0;
                                break;
                            }

                            j++;
                        }

                        if(isValid == 0)
                            continue;

                        CA comm;
                        comm.ut.pid = main.ut.pid;
                        //Adiciona saldo
//                        printf("Comando válido!\n");
                        //Envia pedido para o backend
                        strcpy(comm.word,"ADDMONEY");
                        strcpy(comm.secWord,main.ut.nome);
                        comm.number = atoi(comando[1]);
                        int n = write(fd_bknd_fifo,&comm,sizeof(CA));
//                        if(n == sizeof(CA)){
//                            printf("[INFO] Pedi para adicionar %d SOCoins ao meu saldo.\n\n",comm.number);
//                        }
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tadd <valor>\n\n");
                    }
                }

                else if(strcmp(comando[0],"exit")==0){
                    if(c==1) {
                        //Avisa o backend que vai sair e sair
                        CA comm;
                        comm.ut.pid = main.ut.pid;
                        strcpy(comm.ut.nome,main.ut.nome);
                        strcpy(comm.word,"EXIT");
                        int n = write(fd_bknd_fifo,&comm,sizeof(CA));
//                        if(n == sizeof(CA)){
//                            printf("[INFO] Avisei que me ia desconectar.\n\n");
//                        }
                        break;
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\texit\n\n");
                    }
                }

                else{
                    printf("[WARNING] O comando inserido não é válido.\n");
                }
                i=0,j=0,c=0;
            }
        }
        close(fd_hb_fifo);
    }
    else{
        printf("Número de parâmetros inválido!\n");
        printf("Síntaxe esperada: ./frontend <username> <password>\n\n");
    }

    hbdata.para = 0;
    pthread_join(threadHB, NULL);
    pthread_mutex_destroy(&trinco);

    close(fd_bknd_fifo);
    close(fd_cli_fifo);
    unlink(cli_fifo);

    return 0;
}
