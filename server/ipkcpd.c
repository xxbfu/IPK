/***************
 * IPK proj2
 * 
 * @author Adam Julis
 * 
 * Only in our dreams are we free
 *
 ***************/

#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> 
#include <stdbool.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/time.h> //FD macros
#include <errno.h>
#include <signal.h>

#define MAX 512
#define SA struct sockaddr
#define MAX_CLIENTS 30

//global variable

char adress[15] = "0.0.0.0";
int port = 8080;
bool tcp = false;

int sock_client[MAX_CLIENTS];


void check_arguments(int argc, char *const argv[]){
    if (argc != 7){
        if (argc != 2){
            fprintf(stderr, "Wrong number of arguments, try ./ipkcpc --help for help\n");
            exit(1);
        }
        else if (!(strcmp(argv[1], "-h") && strcmp (argv[1],"--help"))){
            printf("Toto je napoveda, pro vice informaci volej ./ipkcpc --help hahahaha\n -h <host>: IPv4 address of the server\n -p <port>: the server port\n -m <mode>: either 'tcp' or 'udp'\n");
            exit(0);
        }
        else{
            fprintf(stderr, "Wrong number of arguments, try ./ipkcpc --help for help\n");
            exit(1);
        }

    }
    int option, len;
    extern char *optarg;
    extern int optind, opterr, optopt;  //unused
    while ((option = getopt(argc, argv, ":h:p:m:")) != -1){
        switch (option){
            case 'h':
                strcpy(adress,optarg);       //check valid format
                break;
            case 'p':
                len = strlen(optarg);
                for (int i=0;i<len;i++){
                    if (!isdigit(optarg[i])){
                        printf("Wrong format of port\n");
                        exit(1);
                    }
                }
                port = atoi(optarg);      
                if (port < 1 || port > 65535){
                        printf("Wrong size of port\n");
                        exit(1);
                }
                break;
            case 'm':
                if (!(strcmp(optarg, "tcp"))){     
                    tcp = true;
                }
                else if (!(strcmp(optarg, "udp"))){
                    tcp = false;
                }
                else
                {
                    printf("Wrong argument of -m (choose between udp and tcp)\n");
                    exit(1);
                }
                break;
            case ':':
            case '?':
                printf("errors in switch in check arg \n");
                exit(1);
        }
    }
};

void sigint_handler(int sig){

    printf("ahoj hura koncime, control c si mackame\n");
    for(int i = 0; i < MAX_CLIENTS;i++){

        if(sock_client[i] != 0){
            send(sock_client[i], "BYE\n", 5, MSG_DONTWAIT);
            close(sock_client[i]);
        }
    }
    exit(0);
}

/** Function for recieveing chars from open socket until '\n' char
*
* @param
* @return 0: OK, 1: end of connection, -1: error of recv function
*/
int read_while_end_line(int socket, char *buff){
    char tmp_buff = '0';
    int state;
    int cnt_tmp_buff = 0;
    bzero(buff, MAX);
    while(tmp_buff != '\n'){
        if (!(state = recv(socket, &tmp_buff, 1, MSG_NOSIGNAL))){
            return 1;
        }
        else if(state == -1){
            
            printf("Chyba pri cteni socketu\n");
            exit(EXIT_FAILURE); 
        }
        if (tmp_buff == '\n' ){
            break;
        }
        else {
            buff[cnt_tmp_buff] = tmp_buff;
            cnt_tmp_buff++;
        }
    }
    return 0;
};

/**
 * 
 * Function for counting, basic switch and pretend 0 divide
 * 
*/
int count(char operator, int a, int b) {
    switch (operator) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': 
            if (b == 0)
                return -1;
            return a / b;
         default: return -1;
    }
}

/**
 * ach ta rekurze...
 * 
 * Counting function for UDP and TCP communication, "heart" of calculator, doesnt follow 100% the protocol
 * 
 * Function Doesnt work 100%, i spend so much time with that iam sorry...
 * 
 * School of pointers
 * 
 * @return -1 if something wrong or result (results cant be negative)
 * 
*/
int solve(char **input, int *error) {
    char operator;
    int result;

    //skipuje white spaces, v lehkem rozporu s rfc 5234 ale prijde mi to intuitivnejsi
    while (isspace(**input)) {
        (*input)++;
    }

    //lookin for digit or negative
    if (isdigit(**input) || (**input == '-')) {
        result = strtol(*input, input, 10);
    }
    
    //at least we have coloumbs
    else if (**input == '(') {
        (*input)++;
        operator = *(*input)++;
        result = solve(input, error);
        
        //error capture for stop unpredictable freezing
        if (*error) {
            return -1;
        }

        //Go until end of the string or coloumb
        while (**input != ')' && **input != '\0') {
            
            //Vnorovani v zavorkach
            int next_value = solve(input, error);

            //error capture for stop unpredictable freezing
            if (*error) {
                return -1;
            }

            // finally sum results
            result = count(operator, result, next_value);

            if (result == -1) {
                *error = 1;
                return -1;
            }
        }

        //cant be end of string now
        if (**input == '\0') {
            *error = 1;
            return -1;
        }
        (*input)++;
    }

    else {
        *error = 1;
        return -1;
    }
    return result;
}

int main(int argc, char *argv[]){
    
    //Handler for capturing ctrl+c
    signal(SIGINT,sigint_handler);

    check_arguments(argc, argv);
   // printf("Adresa: %s, port: %d a nakonec tcp je %s\n", adress, port, tcp ? "true": "false");


    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;
    char buff[MAX];
    //tcp variant
   if (tcp){
    
        //set to 0, it ll be used for multiple connection
        for (int i = 0; i < MAX_CLIENTS; i++){
            sock_client[i] = 0;
        }

        // socket create
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd <= 0) {
            printf("socket creation failed...\n");
            exit(0);
        }
        else
            printf("Socket successfully created..\n");
        
        int enable = 1;

        //Good habit
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
        
        //We want to be clear as much as positionsible >D 
        memset(&servaddr, 0, sizeof(servaddr));
        

        // assign IP (GLOBAL VARIABLE ADRESS), PORT
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(adress);
        servaddr.sin_port = htons(port);
        
        struct sockaddr *address = (struct sockaddr *) &servaddr;
        int address_size = sizeof(servaddr);

        // Binding newly created socket to given IP and verification
        if ((bind(sockfd, (SA*)&servaddr, address_size)) < 0) {
            printf("socket bind failed...1...\n");
            exit(0);
        }
        else
            printf("Socket successfully binded..\n");
    
        // Now server is ready to listen and verification, MAX five pending connections
        if ((listen(sockfd, 5)) != 0) {
            printf("Listen failed...\n");
            exit(0);
        }
        else
            printf("Server listening..\n");
        
        //descriptor of sockets, used for multiple clients comm.
        fd_set monitoring;
        socklen_t addrr_size = sizeof(address);

        //variable for multi client function
        int socket, top_socket, running;
        int comm_socket, tmp_read, result, position;

        char tmp_buff = '0'; int cnt_tmp_buff; int state;

        //Main cyclus
        while(1){

            //set to 0
            FD_ZERO(&monitoring);
            
            FD_SET(sockfd, &monitoring);
            top_socket = sockfd;
            
            //setting childen sockets
            for (int i = 0; i < MAX_CLIENTS; i++){
                
                socket = sock_client[i];

                if (socket){
                    FD_SET(socket, &monitoring);
                }

                if (socket > top_socket){
                    top_socket = socket;
                }
            }
            
            //NULL - SET TIMEOUTS!!!
            running = select(top_socket + 1, &monitoring, NULL, NULL, NULL);

            //check select
            if ((running < 0) && (errno != EINTR )){
                printf("error while select\n");
            }
            
            //BIG IF STARTED
            //incomming connection
            if (FD_ISSET(sockfd, &monitoring)){
                if ((comm_socket = accept(sockfd, address, &addrr_size)) < 0 ){
                    printf("Error accept\n");
                    exit(EXIT_FAILURE);
                }
                //inform of socket number 
                printf("New connection, socket fd is %d, ip is : %s, port : %d\n" , comm_socket , inet_ntoa(servaddr.sin_addr) , ntohs(servaddr.sin_port));
 
                bzero(buff, MAX);
                cnt_tmp_buff = 0;
                tmp_buff = 0;

                //just look at name of function >D
                if(read_while_end_line(comm_socket, buff)){
                    printf("Konec spojeni od clienta behem cteni HELLO\n");
                    close(comm_socket);
                    continue;
                }

                if (!strcmp(buff, "HELLO")){

                    //send new connection message
                    if( send(comm_socket, "HELLO\n", 7, 0) != 7 ) 
                    {
                        perror("send\n");
                    }
                }
                else{

                    //funny comment
                    //tohle nebude fungovat, nejake goto vymyslet xdd
                    //it works, really
                    if( send(comm_socket, "BYE\n", 5, 0) != 5 ) 
                    {
                        perror("send\n");
                    }
                    close(comm_socket);
                    printf("Error while first hello message\n");
                    //w8 for next connection on next loop
                    continue;
                }

                //add comm socket to clientt socket
                for (int i = 0; i < MAX_CLIENTS; i++){
        
                    if(sock_client[i] == 0){
                        sock_client[i] = comm_socket;
                        printf("New socket has number %d and positionition %d\n",comm_socket, i);
                        break;
                    }
                }
            }
            //BIG IF ENDED

            //existing connection
            for (int i = 0; i < MAX_CLIENTS; i++) {

                socket = sock_client[i];
                bzero(buff, MAX);

                if (FD_ISSET( socket , &monitoring)) {
                    
                    //Check if it was for closing , and also read the incoming message
                    if(read_while_end_line(socket, buff)){
                        getpeername(socket , (struct sockaddr*)&address , (socklen_t*)&address_size);
                        printf("Host with ip %s on port %d disconnected\n" , inet_ntoa(servaddr.sin_addr) , ntohs(servaddr.sin_port));
                        close(socket);
                        sock_client[i] = 0;
                    }

                    //Echo back the message that came in
                    else
                    {
                        
                        position = 6;

                        //Doesnt matther how much white spaces or missclick user do
                        char *input = strstr(buff, "SOLVE");
                        int result, error = 0;

                        //start with '(', solve the big math and send it
                        if (input) {
                            input += strlen("SOLVE ");
                            result = solve(&input, &error);

                            //check the result
                            if ((error)||(result == -1)){
                                printf("Chybny formát\n");
                                send(socket , "BYE\n" , 5 , 0 );
                                close(socket);
                                sock_client[i] = 0;
                                continue;
                            }
                            printf("Výsledek: %d\n", result);
                        }
                        else {
                            printf("Retezec neobsahuje klíčové slovo 'SOLVE'.\n");
                            send(socket,"BYE\n",5 , 0 );
                            close(socket);
                            sock_client[i] = 0;
                            printf("Ending socket with id %d and positionition %d\n",socket, i);
                        }

                        //If everything seems fine, send the result
                        {
                            bzero(buff, MAX);
                            sprintf(buff, "RESULT %d\n", result);
                            printf(".%s.", buff);
                            send(socket , buff , strlen(buff) , 0 );
                        }    
                    }
                }
            }
        }    
   }

   //UDP
   else{

        //Making UDP socket
        if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) <= 0){
            perror("Tvorba UDP socket neuspesna\n");
            exit(EXIT_FAILURE);
        }

        //Init
        memset(&servaddr, 0, sizeof(servaddr));
        memset(&cli, 0, sizeof(cli));

        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(port);

        struct sockaddr *address = (struct sockaddr *) &servaddr;
        int address_size = sizeof(servaddr);

        int incom_len, response_len, result, err;

        char tmp_buff[MAX];
        unsigned char send_buff[MAX];
        len = sizeof(cli);

        if (bind(sockfd, (SA*)&servaddr,address_size) < 0){
            printf("Socket bind failed\n");
            exit(EXIT_FAILURE);
        }
        else
            printf("Socket successfully bind\n");
        
        while(1){

            
            bzero(buff, MAX);
            bzero(tmp_buff, MAX);
            bzero(send_buff, MAX);
            memset(&cli, 0, len);
            err = 0;
            result = 0;

            // receive message from client
            int n = recvfrom(sockfd, (char *)buff, MAX, MSG_WAITALL, (struct sockaddr *)&cli, &len);
            if(buff[0] != '\0'){
                continue;
            }

            //prevent seg faul
            incom_len = buff[1];
            if (incom_len >= MAX - 2){
                incom_len = MAX-2;
            }
            
            //load message to buff
            for (int i = 0; i < incom_len; i++){
                tmp_buff[i] = buff[i + 2];
            }

            //retype for solve function
            char * input = tmp_buff;

            result = solve(&input, &err);

            //Check if solve doesnt detect the error, if yes, send ERR and reset the main cyclus
            if ((err)||(result == -1)){

                printf("Chybny formát\n");
                send_buff[0] = '\1';
                send_buff[1]= '\1';
                sendto(sockfd, send_buff, 2, 0, (struct sockaddr *) &cli, address_size); //nepouzivat msg_confirm
                continue;
            }
                                                                        
            bzero(tmp_buff,MAX);
            
            //Write int to buffer as string
            sprintf(tmp_buff,"%d",result);
            response_len = strlen(tmp_buff);

            //Set heading of socket
            send_buff[0] = '\1';
            send_buff[1]= '\0';
            send_buff[2]= response_len;

            //Add the result to the buffer for send (starting on 3 byte)
            for (int i = 3; i < response_len + 3; i++){
                send_buff[i] = tmp_buff[i-3];
            }
            
            //Send answer to client
            sendto(sockfd, send_buff, strlen(send_buff)+3, 0, (struct sockaddr *) &cli, address_size);
            printf("Response sent\n");
        }
    }

    return 0;    

};