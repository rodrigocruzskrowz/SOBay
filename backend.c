#include "backend.h"

int parar = 0;
int pidf;

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
        while((nbytes = read(fd_init,&c,1)) > 0){
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

int main() {

    initPlataforma();

    int fd_sv_fifo;
    int fd_cli_fifo;
    char res_cli_fifo[MAX_SIZE_FIFO];
    int dados;
    User user;

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

    //Verifica se a variavel de ambiente FUSERS existe
    if(getenv("FUSERS") == NULL){
        printf("A variável de ambiente 'FUSERS' não foi definida.\n");
        exit(1);
    }
    else{
        FUSERS = getenv("FUSERS");
        printf("\nVariável de ambiente 'FUSERS' = %s\n",FUSERS);
    }

    //Acede ao ficheiro de utilizadores
    int nUtilizadores = loadUsersFile(FUSERS);
    printf("\nNum. Utilizadores no ficheiro: %d\n",nUtilizadores);

    //Cria BACKEND FIFO
    mkfifo(BKND_FIFO,0600);
    fd_sv_fifo = open(BKND_FIFO,O_RDWR);

    //Lê dados de login enviados pelo frontend
    dados = read(fd_sv_fifo,&user,sizeof(User));
    if(dados == sizeof(User)){
        printf("Logs recebidos: %s %s\n",user.nome,user.password);
    }

    //Valida dados de login
    user.valid = isUserValid(user.nome,user.password);
    if(user.valid == 0){
        sprintf(res_cli_fifo,FRND_FIFO,user.pid);
        fd_cli_fifo = open(res_cli_fifo,O_WRONLY);
        write(fd_cli_fifo,&user,sizeof(User));
        close(fd_cli_fifo);
    }
    else if(user.valid == 1){
        user.saldo = getUserBalance(user.nome);
        sprintf(res_cli_fifo,FRND_FIFO,user.pid);
        fd_cli_fifo = open(res_cli_fifo,O_WRONLY);
        write(fd_cli_fifo,&user,sizeof(User));
        close(fd_cli_fifo);
    }
    else{
        printf("[ERRO] %s\n", getLastErrorText());
    }

    Item item_lista[MAX_ITEMS];
    int nitems_lista = 0;
    int opc;
    int estado;
    do {
        printf("\nDeseja testar qual opção?\n");
        printf("1-Comandos\n");
        printf("2-Execução do promotor\n");
        printf("3-Utilizadores\n");
        printf("4-Items\n");
        printf("5-Coloca item à venda\n");
        printf("0-Terminar\n");
        printf("Opção: ");
        scanf("%d", &opc);
        switch (opc) {
            case 1: {
                //Espera comando
                char str[MAX_SIZE] = "";
                char comando[6][MAX_SIZE];
                while (1) {
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
                            exit(1);
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
                break;
            }

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

                int fd, cont = 0, arg = 0, nbytes, res, i = 0;
                char str[MAX_SIZE];
                char c;

                fd = open(FITEMS, O_RDONLY);
                if(fd==-1){
                    printf("\nFicheiro de items não encontrado! Não existem items listados para venda.\n");
                    break;
                }
                else{
                    printf("\nInformação do ficheiro: \n");
                    while((nbytes = read(fd,&c,1)) >= 0){ //le byte por byte
                        //printf("%c",c);
                        if(c == '\n' || nbytes == 0){
                            str[cont++]='\0';
                            if(sscanf(str,"%d %s %s %d %d %d %s %s",
                                      &item_lista[i].id,item_lista[i].nome,item_lista[i].categoria,&item_lista[i].bid,&item_lista[i].buyNow,&item_lista[i].tempo,item_lista[i].vendedor,item_lista[i].licitador) == 8){
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
                break;
            }
            case 5:{
                //Lê dados do item
                Item it;
                int lido = read(fd_sv_fifo,&it,sizeof(Item));
                if(lido == sizeof(Item)){
                    printf("\nItem recebido:\nId: %d\nNome: %s\nCategoria: %s\nPreço atual: %d\nPreço compre já: %d\nTempo até fim de leilão: %d\nVendedor: %s, Licitador: %s\n",
                           it.id,it.nome,it.categoria,it.bid,it.buyNow,it.tempo,it.vendedor,it.licitador);

                    //Coloca item à venda (adiciona à lista de items)
                    item_lista[nitems_lista].id = PROX_ID;
                    strcpy(item_lista[nitems_lista].nome, it.nome);
                    strcpy(item_lista[nitems_lista].categoria, it.categoria);
                    item_lista[nitems_lista].bid = it.bid;
                    item_lista[nitems_lista].buyNow = it.buyNow;
                    item_lista[nitems_lista].tempo = it.tempo;
                    strcpy(item_lista[nitems_lista].vendedor, it.vendedor);
                    strcpy(item_lista[nitems_lista].licitador, it.licitador);
                    PROX_ID++;
                    nitems_lista++;
                }

                printf(":::::LISTA DE ITEMS:::::");
                for(int i=0; i<nitems_lista; i++){
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

//                int itemsEnviados = 0;
//                for(int i=0; i<nitems_lista; i++){
//                    int n = write(fd_cli_fifo,&item_lista[i],sizeof(Item));
//                    if(n == sizeof(Item)){
//                        itemsEnviados++;
//                        //printf("[INFO] Enviei %d %s %s %d %d %d %s %s\n\n",it.id,it.nome,it.categoria,it.bid,it.buyNow,it.tempo,it.vendedor,it.licitador);
//                    }
//                }
//                printf("Enviei %d items.",itemsEnviados);

                break;
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

    close(fd_cli_fifo);
    close(fd_sv_fifo);
    unlink(res_cli_fifo); //Fecha canal do cliente
    unlink(BKND_FIFO); //Fecha canal do servidor
}