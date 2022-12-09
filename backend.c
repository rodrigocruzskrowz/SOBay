#include "backend.h"
#include "frontend.h"
#include "utils.h"

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

int main() {

    int opc;
    int estado;
    do {
    printf("Deseja testar qual opção?\n");
    printf("1-Comandos\n");
    printf("2-Execução do promotor\n");
    printf("3-Utilizadores\n");
    printf("4-Items\n");
    printf("5-Verificar login\n");
    printf("6-Terminar\n");
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
                //Verifica se a variavel de ambiente FPROMOTERS existe
                if(getenv("FPROMOTERS") == NULL){
                    printf("A variável de ambiente 'FPROMOTERS' não foi definida.\n");
                    exit(1);
                }
                else{
                    FPROMOTERS = getenv("FPROMOTERS");
                    printf("\nVariável de ambiente 'FPROMOTERS' = %s\n\n",FPROMOTERS);
                }

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
                        if(c == ' ' || c == '\n' || c == EOF){
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

                    printf("[ERRO] Não consegui executar o promotor!\n");
                    break;
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
//                                printf("%s",s);
//                                memset(s,0,MAX_SIZE);
//                                printf("\nESPAÇO//bN\n");
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
                //Verifica se a variavel de ambiente FUSERS existe
                if(getenv("FUSERS") == NULL){
                    printf("A variável de ambiente 'FUSERS' não foi definida.\n");
                    exit(1);
                }
                else{
                    FUSERS = getenv("FUSERS");
                    printf("\nVariável de ambiente 'FUSERS' = %s\n\n",FUSERS);
                }

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
                    exit(1);
                }
                else{
                    FITEMS = getenv("FITEMS");
                    printf("\nVariável de ambiente 'FITEMS' = %s\n\n",FITEMS);
                }

                int fd, cont = 0, arg = 0, nbytes, res, i = 0;
                char str[MAX_SIZE];
                char c;
                item item[MAX_ITEMS];

            fd = open(FITEMS, O_RDONLY);
            if(fd==-1){
                printf("\nFicheiro de items não encontrado!\n");
                break;
            }
            else{
                printf("\nInformação do ficheiro: \n");
                while((nbytes = read(fd,&c,1)) > 0){ //le byte por byte
                    //printf("%c",c);
                    if(c == ' ' || c == '\n' || c == EOF){
                        str[cont++]='\0';
                        cont = 0;
                        switch (arg){
                            case 0:{
                                item[i].id = atoi(str);
                                memset(str,0,MAX_SIZE);
                                break;
                            }
                            case 1:{
                                strcpy(item[i].nome,str);
                                memset(str,0,MAX_SIZE);
                                break;
                            }
                            case 2:{
                                strcpy(item[i].categoria,str);
                                memset(str,0,MAX_SIZE);
                                break;
                            }
                            case 3:{
                                item[i].bid = atoi(str);
                                memset(str,0,MAX_SIZE);
                                break;
                            }
                            case 4:{
                                item[i].buyNow = atoi(str);
                                memset(str,0,MAX_SIZE);
                                break;
                            }
                            case 5:{
                                item[i].tempo = atoi(str);
                                memset(str,0,MAX_SIZE);
                                break;
                            }
                            case 6:{
                                strcpy(item[i].vendedor,str);
                                memset(str,0,MAX_SIZE);
                                break;
                            }
                            case 7:{
                                strcpy(item[i].licitador,str);
                                memset(str,0,MAX_SIZE);
                                arg = -1;
                                break;
                            }
                            default: {
                                printf("[ERRO] Fora dos limites de atributos.");
                                break;
                            }
                        }
                        arg++;

                        if(c == '\n'){
                            printf("\n:::ITEM %d:::\n",i+1);
                            printf("ID: %d\n", item[i].id);
                            printf("Item: %s\n", item[i].nome);
                            printf("Categoria: %s\n", item[i].categoria);
                            printf("Licitação: %d\n", item[i].bid);
                            printf("Compre já: %d\n", item[i].buyNow);
                            printf("Tempo de venda: %d\n", item[i].tempo);
                            printf("Vendedor: %s\n", item[i].vendedor);
                            printf("Licitador: %s\n", item[i].licitador);

                            if(i < MAX_ITEMS)
                                i++;
                            else
                                break;
                        }
                    }
                    else{
                        str[cont++] = c;
                    }
                }
            }
                break;
            }
            case 5:{
                int fd_sv_fifo;
                int fd_cli_fifo;
                char res_cli_fifo[MAX_SIZE_FIFO];
                int dados;
                User user;

                //Verifica se a variavel de ambiente FUSERS existe
                if(getenv("FUSERS") == NULL){
                    printf("A variável de ambiente 'FUSERS' não foi definida.\n");
                    exit(1);
//                    FUSERS = "../users.txt";
                }
                else{
                    FUSERS = getenv("FUSERS");
                    printf("\nVariável de ambiente 'FUSERS' = %s\n\n",FUSERS);
                }

                int nUtilizadores = loadUsersFile(FUSERS);
                printf("\nNum. Utilizadores no ficheiro: %d\n",nUtilizadores);

                if(access(BKND_FIFO,F_OK) == 0){
                    printf("O BACKEND já está em execução!\n");
                    exit(1);
                }
                mkfifo(BKND_FIFO,0600);
                fd_sv_fifo = open(BKND_FIFO,O_RDWR);
                struct sigaction sa;
                    sa.sa_handler = stopValidatingLogs;
                    sa.sa_flags = SA_SIGINFO;
                sigaction(SIGINT,&sa,NULL);

                parar = 0;
                while(parar == 0){
                    dados = read(fd_sv_fifo,&user,sizeof(User));

                    if(dados == sizeof(User)){
                        printf("Logs recebidos: %s %s\n",user.nome,user.password);
                    }

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
                }

                close(fd_cli_fifo);
                close(fd_sv_fifo);
                unlink(res_cli_fifo);

                break;
            }
            case 6:{
                return 0;
            }
            default:
                printf("Opção desconhecida.");
                break;
        }
        printf("\n\n\n");
    }while(opc!=5);
}