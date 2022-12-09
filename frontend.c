#include "frontend.h"
#include "utils.h"

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
            printf("O BACKEND não está em execução.\n");
            exit(1);
        }
        fd_bknd_fifo = open(BKND_FIFO,O_WRONLY);
        if(fd_bknd_fifo == -1){
            printf("Não foi possível abrir o canal de comunicação com o BACKEND.\n");
            exit(1);
        }

        //Cria FIFO do cliente
        user.pid = getpid();
        sprintf(cli_fifo,FRND_FIFO,user.pid);
        if(access(cli_fifo,F_OK) != 0)
            mkfifo(cli_fifo,0600);
        fd_cli_fifo = open(cli_fifo,O_RDWR);
        if(fd_cli_fifo == -1){
            printf("Não foi possível criar o canal de comunicação com o BACKEND.\n");
            exit(1);
        }

        //Envia login para o BACKEND
        int n = write(fd_bknd_fifo,&user,sizeof(User));
//        if(n == sizeof(User)){
//            printf("Enviei %s %s %d\n",user.nome, user.password,user.pid);
//        }

        //Recebe validação do login
        int resposta;
        resposta = read(fd_cli_fifo,&user,sizeof(User));
        if(resposta == sizeof(User)){
            if(user.valid == 1){
                printf("Olá, %s!\nO seu saldo é de: %d\n\n",user.nome,user.saldo);
            }
            else{
                printf("Credênciais erradas. Acesso negado!\n\n");
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
                printf("[ERRO] ERRO NO SELECT - %s\n", strerror(errno));
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
                        int isValid = 1;
                        //Verificar se o item existe

                        //Verificar se a categoria existe

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
                            continue;close(fd_bknd_fifo);
                close(fd_cli_fifo);
                unlink(cli_fifo);

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

                        printf("Comando válido!\n");

                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tsell <nome-item> <categoria> <preço-base> <preço-compre-já> <duração>\n\n");
                    }
                }

                else if(strcmp(comando[0],"list")==0){
                    if(c==1) {
                        //Listar items
                        printf("Comando válido!\n");
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tlist\n\n");
                    }
                }

                else if(strcmp(comando[0],"licat")==0){
                    if(c==2) {
                        //Verificar se categoria existe

                        //Listar Items da categoria
                        printf("Comando válido!\n");
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tlicat <nome-categoria>\n\n");
                    }
                }

                else if(strcmp(comando[0],"lisel")==0){
                    if(c==2) {
                        //Verificar se vendedor existe

                        //Listar Items do vendedor
                        printf("Comando válido!\n");
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tlisel <nome-vendedor>\n\n");
                    }
                }

                else if(strcmp(comando[0],"lival")==0){
                    if(c==2) {
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
                        printf("Comando válido!\n");
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tlival <preço-máximo>\n\n");
                    }
                }

                else if(strcmp(comando[0],"litime")==0){
                    if(c==2) {
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

                        //Listar items com prazo até tempo indicado
                        printf("Comando válido!\n");
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tlitime <hora-em-segundos>\n\n");
                    }
                }

                else if(strcmp(comando[0],"time")==0){
                    if(c==1) {
                        //Indicar hora atual
                        printf("Comando válido!\n");
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

                        //Licitar item
                        printf("Comando válido!\n");
                    }
                    else {
                        printf("[WARNING] O comando inserido não é válido.\n");
                        printf("Formato esperado:\n\tbuy <id> <valor>\n\n");
                    }
                }

                else if(strcmp(comando[0],"cash")==0){
                    if(c==1) {
                        //Mostra saldo
                        printf("Comando válido!\n");
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

                        //Adiciona saldo
                        printf("Comando válido!\n");
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