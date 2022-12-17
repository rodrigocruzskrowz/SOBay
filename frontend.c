#include "frontend.h"

void imprimeItem(Item *it, int *i){
    printf("\n:::ITEM %d:::\n",*i+1);
    printf("ID: %d\n", it->id);
    printf("Item: %s\n", it->nome);
    printf("Categoria: %s\n", it->categoria);
    printf("Licitação: %d\n", it->bid);
    printf("Compre já: %d\n", it->buyNow);
    printf("Tempo de venda: %d\n", it->tempo);
    printf("Vendedor: %s\n", it->vendedor);
    printf("Licitador: %s\n", it->licitador);
}

int main(int argc, char *argv[])
{
    User user;
    int fd_bknd_fifo;
    int fd_cli_fifo;
    char cli_fifo[MAX_SIZE_FIFO];
    if(argc == 3){
        //Inicializa estrutura do utilizador
        strcpy(user.nome,argv[1]);
        //user.nome = argv[1];
        strcpy(user.password,argv[2]);
        //user.password = argv[2];
        user.saldo = -1;
        user.valid = 0;
        user.pid = -1;

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

        //Cria FIFO do cliente
        user.pid = getpid();
        sprintf(cli_fifo,FRND_FIFO,user.pid);
        if(access(cli_fifo,F_OK) != 0)
            mkfifo(cli_fifo,0600);
        fd_cli_fifo = open(cli_fifo,O_RDWR);
        if(fd_cli_fifo == -1){
            printf("[ERROR] Não foi possível criar o canal de comunicação com o BACKEND.\n");
            exit(1);
        }

        //Envia login para o BACKEND
        int n = write(fd_bknd_fifo,&user,sizeof(User));
//        if(n == sizeof(User)){
//            printf("[INFO] Enviei %s %s %d\n",user.nome, user.password,user.pid);
//        }

        //Recebe validação do login
        int resposta;
        resposta = read(fd_cli_fifo,&user,sizeof(User));
        if(resposta == sizeof(User)){
            if(user.valid == 1){
                printf("Olá, %s!\nO seu saldo é de: %d SOCoins.\n\n",user.nome,user.saldo);
            }
            else{
                printf("[INFO] Credênciais erradas. Acesso negado!\n\n");
                close(fd_bknd_fifo);
                close(fd_cli_fifo);
                unlink(cli_fifo);
                exit(1);
            }
        }

        //Select
        fd_set fd;
        int res_sel;
        struct timeval timeout;

        int printComando = 1;
        while(1){
            if(printComando == 1) {
                printf("\ncomando > ");
                fflush(stdout);
                printComando = 0;
            }

            FD_ZERO(&fd);
            FD_SET(0,&fd);
            FD_SET(fd_cli_fifo, &fd);
            timeout.tv_sec = 10;
            timeout.tv_usec = 0;

            res_sel = select(fd_cli_fifo + 1, &fd, NULL, NULL, &timeout);

            if(res_sel == -1){
                printf("[ERROR] ERRO NO SELECT - %s\n", strerror(errno));
                break;
            }
            if(res_sel > 0 && FD_ISSET(fd_cli_fifo, &fd)){
                //Trata dados recebidos

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
                        comm.pid = user.pid;
                        Item it;
                        int isValid = 1;

                        strcpy(it.nome,comando[1]);
                        strcpy(it.categoria,comando[2]);

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

                        it.bid = atoi(comando[3]);

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

                        it.buyNow = atoi(comando[4]);

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

                        it.tempo = atoi(comando[5]);

//                        printf("Comando válido!\n");

                        //Informa o backend de que quer criar item
                        strcpy(comm.word,"CRIAR");
                        write(fd_bknd_fifo,&comm,sizeof(CA));

                        //Envia Item para o backend
                        it.id = 0;
                        strcpy(it.vendedor,user.nome);
                        strcpy(it.licitador,"-");
                        int n = write(fd_bknd_fifo,&it,sizeof(Item));
                        if(n == sizeof(Item)){
                            printf("[INFO] Enviei %d %s %s %d %d %d %s %s\n\n",it.id,it.nome,it.categoria,it.bid,it.buyNow,it.tempo,it.vendedor,it.licitador);
                        }

                        //Recebe confirmação do registo do Item
                        int resposta = read(fd_cli_fifo,&comm,sizeof(CA));
                        if(resposta == sizeof(CA)){
                            if(strcmp(comm.word,"INSERIDO") == 0){
                                printf("[INFO] O artigo foi adicionado ao leilão com o id %d!\n\n",comm.secNumber);
                            }
                            else{
                                printf("[ERROR] Erro ao registar o item. Por favor tente mais tarde.\n\n");
                            }
                        }
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tsell <nome-Item> <categoria> <preço-base> <preço-compre-já> <duração>\n\n");
                    }
                }

                else if(strcmp(comando[0],"list")==0){
                    if(c==1) {
                        CA comm;
                        comm.pid = user.pid;

                        Item it;

                        //Listar items
//                        printf("Comando válido!\n");

                        //Envia pedido para o backend
                        strcpy(comm.word,"LISTAR");
                        int n = write(fd_bknd_fifo,&comm,sizeof(CA));
                        if(n == sizeof(Item)){
                            printf("[INFO] Pedi para '%s' items.\n\n",comm.word);
                        }

                        //Recebe resposta
                        int resposta = read(fd_cli_fifo,&comm,sizeof(CA));
                        if(resposta == sizeof(CA)){
                            //printf("Recebi: %s",comm.word);
                            if(strcmp(comm.word,"ENVIADO")==0){
                                //Recebe lista de items
                                printf("\n\n:::::LISTA DE ITEMS:::::");
                                for(int i=0; i<comm.number; i++){
                                    resposta = read(fd_cli_fifo,&it,sizeof(Item));
                                    if(resposta == sizeof(Item)){
                                        imprimeItem(&it,&i);
                                    }
                                }
                            }
                            else{
                                printf("[ERROR] Erro no pedido. Por favor tente mais tarde.\n");
                            }
                        }
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tlist\n\n");
                    }
                }

                else if(strcmp(comando[0],"licat")==0){
                    if(c==2) {
                        CA comm;
                        comm.pid = user.pid;
                        Item it;
                        //Listar Items da categoria
//                        printf("Comando válido!\n");

                        //Envia pedido para o backend
                        strcpy(comm.word,"LISTCAT");
                        strcpy(comm.secWord,comando[1]);
                        int n = write(fd_bknd_fifo,&comm,sizeof(CA));
                        if(n == sizeof(CA)){
                            printf("[INFO] Pedi os items da categoria: '%s'\n\n",comm.secWord);
                        }

                        //Recebe resposta do backend
                        int resposta = read(fd_cli_fifo,&comm,sizeof(CA));
                        if(resposta == sizeof(CA)){
                            if(strcmp(comm.word,"ENVCAT")==0){
                                if(comm.number <= 0){
                                    printf("[INFO] Não existem items da categoria '%s' em leilão.\n",comm.secWord);
                                }
                                else{
                                    //Recebe lista de items
                                    printf("\n\n:::::LISTA DE ITEMS:::::");
                                    for(int i=0; i<comm.number; i++){
                                        resposta = read(fd_cli_fifo,&it,sizeof(Item));
                                        if(resposta == sizeof(Item)){
                                            imprimeItem(&it,&i);
                                        }
                                    }
                                }
                            }
                            else{
                                printf("[ERROR] Erro no pedido. Por favor tente mais tarde.\n");
                            }
                        }
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tlicat <nome-categoria>\n\n");
                    }
                }

                else if(strcmp(comando[0],"lisel")==0){
                    if(c==2) {
                        CA comm;
                        comm.pid = user.pid;
                        Item it;
                        //Listar Items do vendedor
//                        printf("Comando válido!\n");

                        //Envia pedido para o backend
                        strcpy(comm.word,"LISTSEL");
                        strcpy(comm.secWord,comando[1]);
                        int n = write(fd_bknd_fifo,&comm,sizeof(CA));
                        if(n == sizeof(CA)){
                            printf("[INFO] Pedi os items do vendedor: '%s'\n\n",comm.secWord);
                        }

                        //Recebe resposta do backend
                        int resposta = read(fd_cli_fifo,&comm,sizeof(CA));
                        if(resposta == sizeof(CA)){
                            if(strcmp(comm.word,"ENVSEL")==0){
                                if(comm.number <= 0){
                                    printf("[INFO] Não existem items do vendedor '%s' em leilão.\n",comm.secWord);
                                }
                                else{
                                    //Recebe lista de items
                                    printf("\n\n:::::LISTA DE ITEMS:::::");
                                    for(int i=0; i<comm.number; i++){
                                        resposta = read(fd_cli_fifo,&it,sizeof(Item));
                                        if(resposta == sizeof(Item)){
                                            imprimeItem(&it,&i);
                                        }
                                    }
                                }
                            }
                            else{
                                printf("[ERROR] Erro no pedido. Por favor tente mais tarde.\n");
                            }
                        }
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tlisel <nome-vendedor>\n\n");
                    }
                }

                else if(strcmp(comando[0],"lival")==0){
                    if(c==2) {
                        CA comm;
                        comm.pid = user.pid;
                        Item it;
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
                        if(n == sizeof(CA)){
                            printf("[INFO] Pedi os items até %d SOCoins.\n\n",atoi(comando[1]));
                        }

                        //Recebe resposta do backend
                        int resposta = read(fd_cli_fifo,&comm,sizeof(CA));
                        if(resposta == sizeof(CA)){
                            if(strcmp(comm.word,"ENVVAL")==0){
                                if(comm.number <= 0){
                                    printf("[INFO] Não existem items com valor até %d.\n",atoi(comando[1]));
                                }
                                else{
                                    //Recebe lista de items
                                    printf("\n\n:::::LISTA DE ITEMS:::::");
                                    for(int i=0; i<comm.number; i++){
                                        resposta = read(fd_cli_fifo,&it,sizeof(Item));
                                        if(resposta == sizeof(Item)){
                                            imprimeItem(&it,&i);
                                        }
                                    }
                                }
                            }
                            else{
                                printf("[ERROR] Erro no pedido. Por favor tente mais tarde.\n");
                            }
                        }
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tlival <preço-máximo>\n\n");
                    }
                }

                else if(strcmp(comando[0],"litime")==0){
                    if(c==2) {
                        CA comm;
                        comm.pid = getpid();
                        Item it;
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
                        if(n == sizeof(CA)){
                            printf("[INFO] Pedi os items até à hora %d\n\n",atoi(comando[1]));
                        }

                        //Recebe resposta do backend
                        int resposta = read(fd_cli_fifo,&comm,sizeof(CA));
                        if(resposta == sizeof(CA)){
                            if(strcmp(comm.word,"ENVTIM")==0){
                                if(comm.number <= 0){
                                    printf("[INFO] Não existem items em leilão à hora %d.\n",atoi(comando[1]));
                                }
                                else{
                                    //Recebe lista de items
                                    printf("\n\n:::::LISTA DE ITEMS:::::");
                                    for(int i=0; i<comm.number; i++){
                                        resposta = read(fd_cli_fifo,&it,sizeof(Item));
                                        if(resposta == sizeof(Item)){
                                            imprimeItem(&it,&i);
                                        }
                                    }
                                }
                            }
                            else{
                                printf("[ERROR] Erro no pedido. Por favor tente mais tarde.\n");
                            }
                        }
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tlitime <hora-em-segundos>\n\n");
                    }
                }

                else if(strcmp(comando[0],"time")==0){
                    if(c==1) {
                        CA comm;
                        //Indicar hora atual
//                        printf("Comando válido!\n");
                        //Envia pedido para o backend
                        strcpy(comm.word,"TIME");
                        int n = write(fd_bknd_fifo,&comm,sizeof(CA));
                        if(n == sizeof(CA)){
                            printf("[INFO] Pedi a hora atual\n\n");
                        }

                        //Recebe resposta do backend
                        int resposta = read(fd_cli_fifo,&comm,sizeof(CA));
                        if(resposta == sizeof(CA)){
                            if(strcmp(comm.word,"ENVTIME")==0){
                                printf("Hora atual: %d\n",comm.number);
                            }
                            else{
                                printf("[ERROR] Erro no pedido. Por favor tente mais tarde.\n");
                            }
                        }
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
                        strcpy(comm.word,"BUY");
                        strcpy(comm.secWord,user.nome);
                        comm.number = atoi(comando[1]);
                        comm.secNumber = atoi(comando[2]);
                        int n = write(fd_bknd_fifo,&comm,sizeof(CA));
                        if(n == sizeof(CA)){
                            printf("[INFO] Licitei o item %d por %d SOCoins.\n\n",comm.number, comm.secNumber);
                        }
                        //Recebe resposta do backend
                        int resposta = read(fd_cli_fifo,&comm,sizeof(CA));
                        if(resposta == sizeof(CA)){
                            if(strcmp(comm.word,"ERRSALDO")==0){
                                printf("[INFO] O seu saldo não é suficiente para completar a transação.\nO seu saldo é de %d SOCoins.",comm.number);
                            }
                            else if(strcmp(comm.word,"BOUGHT")==0){
                                printf("[INFO] O item %d foi comprado com sucesso!\nO seu saldo atual é de %d SOCoins.\n",atoi(comando[1]),comm.number);
                            }
                            else if(strcmp(comm.word,"BIDDED")==0){
                                printf("[INFO] Licitação de %d colocada no item %d.\nO seu saldo atual é de %d SOCoins.",atoi(comando[2]),atoi(comando[1]),comm.number);
                            }
                            else if(strcmp(comm.word,"ERRID")==0){
                                printf("[INFO] Item com o id %d não encontrado.\n",atoi(comando[1]));
                            }
                            else if(strcmp(comm.word,"ERRVAL")==0){
                                printf("[INFO] O valor de licitação é inferior ao preço atual do item, por favor faça nova licitação.\n");
                            }
                            else{
                                printf("[ERROR] Erro no pedido. Por favor tente mais tarde.\n");
                            }
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
                        //Mostra saldo
//                        printf("Comando válido!\n");
                        //Envia pedido para o backend
                        strcpy(comm.word,"CASH");
                        strcpy(comm.secWord,user.nome);
                        int n = write(fd_bknd_fifo,&comm,sizeof(CA));
                        if(n == sizeof(CA)){
                            printf("[INFO] Pedi para consultar o meu saldo.\n\n");
                        }

                        //Recebe resposta do backend
                        int resposta = read(fd_cli_fifo,&comm,sizeof(CA));
                        if(resposta == sizeof(CA)){
                            if(strcmp(comm.word,"ENVCASH")==0){
                                printf("[INFO] O meu saldo atual: %d SOCoins\n",comm.number);
                            }
                            else{
                                printf("[ERROR] Erro no pedido. Por favor tente mais tarde.\n");
                            }
                        }
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
                        //Adiciona saldo
//                        printf("Comando válido!\n");
                        //Envia pedido para o backend
                        strcpy(comm.word,"ADDMONEY");
                        strcpy(comm.secWord,user.nome);
                        comm.number = atoi(comando[1]);
                        int n = write(fd_bknd_fifo,&comm,sizeof(CA));
                        if(n == sizeof(CA)){
                            printf("[INFO] Pedi para adicionar %d SOCoins ao meu saldo.\n\n",comm.number);
                        }

                        //Recebe resposta do backend
                        int resposta = read(fd_cli_fifo,&comm,sizeof(CA));
                        if(resposta == sizeof(CA)){
                            if(strcmp(comm.word,"ENVADDMONEY")==0){
                                printf("[INFO] Foram adicionadas %d SOCoins ao seu saldo.\nO seu saldo atual é de %d SOCoins.",atoi(comando[1]),comm.number);
                            }
                            else{
                                printf("[ERROR] Erro no pedido. Por favor tente mais tarde.\n");
                            }
                        }
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tadd <valor>\n\n");
                    }
                }

                else if(strcmp(comando[0],"exit")==0){
                    if(c==1) {
                        //Avisa o backend que o cliente saiu

                        //Sair
                        close(fd_bknd_fifo);
                        close(fd_cli_fifo);
                        unlink(cli_fifo);
                        exit(1);
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

        //Verifica se a variavel de ambiente HEARTBEAT existe
        if(getenv("HEARTBEAT") == NULL){
            printf("A variável de ambiente 'HEARTBEAT' não foi definida.\n");
            exit(1);
        }
        else{
            const int HEARTBEAT = atoi(getenv("HEARTBEAT"));
            printf("Variável de ambiente 'HEARTBEAT' = %d\n\n",HEARTBEAT);
        }
    }
    else{
        printf("Número de parâmetros inválido!\n");
        printf("Síntaxe esperada: ./frontend <username> <password>\n\n");
    }

    close(fd_bknd_fifo);
    close(fd_cli_fifo);
    unlink(cli_fifo);

    return 0;
}