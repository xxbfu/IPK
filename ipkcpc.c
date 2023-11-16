/**
 * IPKCPC
 * author: Adam Juliš
 * login: xjulis00
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <regex.h>
#include <stdbool.h>
#include <signal.h>

#define TCP_BUFF 1024
#define UDP_BUFF 255

typedef unsigned char BYTE;
int client_socket;

/**
 * Checks for correct number of arguments
 * @param argc - number of arguments
 */
void arg_check(int argc){
    if (argc != 7 && argc != 2) {
        fprintf(stderr, "Wrong parameters :%d:\n"
                        "Run the program again with single argument -i to show help\n", argc);
        exit(1);
    }
}

/**
 * Sets appropriate arguments if they meet particular conditions
 * @param argv - arguments
 * @param argc - number of arguments
 * @param mode - variable to put value of argument -m into
 * @param server_ip - variable to put value of argument -h into
 * @param port - variable to put value of argument -p into
 */
void arg_set(char ** argv,int argc, char ** mode,char ** server_ip,int * port){
    int c;
    while ((c = getopt(argc, argv, "h:p:m:i")) != -1) {
        switch (c) {
            case 'i':
                if(argc == 2){
                    fprintf(stdout,"IPK Calculator Protocol\n"
                           "-i: print information about the program\n"
                           "-h <host>: IPv4 address of the server\n"
                           "-p <port>: the server port\n"
                           "-m <mode>: either 'tcp' or 'udp'\n"
                           "If chosen mode is TCP user has to start the conversation with HELLO\n"
                           "Both modes can be ended by ctrl+c\n"
                           "Form of the message in TCP mode:\n"
                           "HELLO\nSOLVE ('/'or'*'or'-'or'+' 1st.number 2nd.number)\nBYE\n"
                           "Form of the message in UDB mode:\n"
                           "('/'or'*'or'-'or'+' 1st.number 2nd.number)\n");
                    exit(0);
                }else{
                    fprintf(stderr,"Run the program again with single argument -i to show help\n");
                    exit(1);
                }
            case 'h': {
                if(argc == 7){
                    *server_ip = optarg;
                }
                break;
            }
            case 'p': {
                if(argc == 7){
                    regex_t regex;
                    if (regcomp(&regex, "^[0-9]+$", REG_EXTENDED) == 0) {
                        int status = regexec(&regex, optarg, 0, NULL, 0);
                        if (status == 0) {
                            *port = atoi(optarg);
                        } else {
                            fprintf(stderr, "ERROR wrong syntax of port :%s:\n", optarg);
                            regfree(&regex);
                            exit(1);
                        }
                    } else {
                        fprintf(stderr, "ERROR compiling regex\n");
                        regfree(&regex);
                        exit(1);
                    }
                    regfree(&regex);
                }
                break;
            }
            case 'm': {
                if(argc == 7){
                    regex_t regex;
                    if (regcomp(&regex, "^(tcp|udp)$", REG_EXTENDED) == 0) {
                        if (regexec(&regex, optarg, 0, NULL, 0) == 0) {
                            *mode = optarg;
                        } else {
                            fprintf(stderr, "ERROR wrong mode %s\n", optarg);
                            regfree(&regex);
                            exit(1);
                        }
                    } else {
                        fprintf(stderr, "ERROR compiling regex\n");
                        regfree(&regex);
                        exit(1);
                    }
                    regfree(&regex);
                }
                break;
            }
            default: {
                fprintf(stderr, "ERROR unknown parameter\n"
                                "Run the program again with single argument -i to show help\n");
                exit(1);
            }
        }
    }
}

/**
 *  gets the address of the server with the use of DNS
 * @param server structure with server variable values
 * @param server_ip value of the argument -h
 */
void address(struct hostent **server,char ** server_ip){
    if ((*server = gethostbyname(*server_ip)) == NULL) {
        fprintf(stderr, "ERROR no such host as %s\n", *server_ip);
        exit(1);
    }
}

/**
 * Finds the IP address of the server and initializes the structure server_address
 * @param server - structure with server variable values
 * @param port - value of argument -p
 * @return - the structure server_address
 */
struct sockaddr_in ip_server(struct hostent *server,int port){
    struct sockaddr_in server_address;
    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy((char *) server->h_addr, (char *) &server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(port);
    return server_address;
}

/**
 * creates socket of the particular type
 * @param type - defined by the mode - SOCK_STREAM/SOCK_DGRAM
 */
void create_socket(int type){
    if ((client_socket = socket(AF_INET, type, 0)) <= 0) {
        perror("ERROR socket");
        exit(1);
    }
}

/**
 * Establishes connection of the socket to our server
 * @param server_address - information about server
 */
void establish_connection(struct sockaddr_in server_address){
    if (connect(client_socket, (const struct sockaddr *) &server_address, sizeof(server_address)) != 0) {
        perror("ERROR connect");
        close(client_socket);
        exit(1);
    }
}

/**
 * This function sends receives and prints information in mode TCP
 * is separately so we could call it in case of SIGINT signal
 * @param buffer - users input
 * @return - 0 to continue connection,1 to close the connection
 */
int tcp_srp(char buffer[]){
    if ((send(client_socket, buffer, strlen(buffer), 0)) < 0) {
        perror("ERROR send");
        close(client_socket);
        exit(1);
    }
    bzero(buffer, TCP_BUFF);
    if ((recv(client_socket, buffer, TCP_BUFF, 0)) < 0){
        perror("ERROR in recvfrom");
        return 1;
    }
    fprintf(stdout,"%s", buffer);
    if (strcmp(buffer, "BYE\n") == 0) {
        return 1;
    }
    return 0;
}

/**
 * This function is called in TCP mode when user sent SIGINT signal (ctrl+c)
 * sends message BYE so the server could close the connection properly
 */
void sigintHandler_tcp(int) {
    char buffer[] = "BYE\n";
  // printf("\n%s",buffer);
    tcp_srp(buffer);
    close(client_socket);
    exit(0);
}

/**
 * This function is called in UDP mode when user sent SIGINT signal (ctrl+c)
 */
void sigintHandler(int) {
    exit(0);
}

/**
 * Main functionality in TCP mode from reading user input to printing out the response
 */
void tcp_loop(){
    bool first = 1;
    char buffer[TCP_BUFF];
    if (signal(SIGINT, sigintHandler_tcp) == SIG_ERR) {
        exit(0);
    }
    while (1) {
        bzero(buffer, TCP_BUFF);
        if ((fgets(buffer, TCP_BUFF, stdin)) == NULL) {
            perror("ERROR fgets");
            exit(1);
        }
        // for CRLF ending
        if(buffer[strlen(buffer) -1] == '\r'){
            buffer[strlen(buffer) -1] = '\n';
        }

        if (first && strcmp(buffer, "HELLO\n") != 0 ) {
            fprintf(stderr, "ERROR starting conversation with server\n"
                            "Run the program again with single argument -i to show help\n");
            close(client_socket);
            exit(1);
        }
        first = 0;
        if(tcp_srp(buffer)){
            break;
        }
    }
}

/**
 * Main functionality in UDP mode from reading user input to printing out the response
 */
void udp_loop(struct sockaddr_in server_address){
    if (signal(SIGINT, sigintHandler) == SIG_ERR) {
        exit(0);
    }
    while (1) {
        char buffer[UDP_BUFF];
        bzero(buffer, UDP_BUFF);
        if ((fgets(buffer, UDP_BUFF, stdin)) == NULL) {
            perror("ERROR fgets");
            exit(1);
        }
        buffer[strlen(buffer) - 1] = '\0';
        BYTE out[strlen(buffer) + 2];
        out[0] = '\0';
        out[1] = strlen(buffer);

        for (int i = 0; i < strlen(buffer); i++) {
            out[i + 2] = buffer[i];
        }

        socklen_t serverlen = sizeof(server_address);
        if (sendto(client_socket, out, strlen(buffer) + 2, 0, (struct sockaddr *) &server_address, serverlen) < 0) {
            perror( "ERROR send");
            exit(1);
        }
        bzero(buffer, UDP_BUFF);
        if (recvfrom(client_socket, (char *) buffer, UDP_BUFF, MSG_WAITALL, (struct sockaddr *) &server_address,&serverlen) < 0) {
            perror("ERROR receive");
            exit(1);
        }
        if (buffer[0] == 1) {
            if (buffer[1] == 0) {
                fprintf(stdout,"OK:");
            } else {
                fprintf(stdout,"ERR:");
            }
            //fprintf(stdout,"Jsem v cyklu %d\n", buffer[2]);
            for (int i = 3; i < 3 + buffer[2]; i++) {
                fprintf(stdout,"%c", (char) buffer[i]);
               // fprintf(stdout,"Jsem v cyklu chachaa\n");
            }
            fprintf(stdout,"\n");
        }

    }
}

/**
 * Call all functions and runs the program
 * @param argc - number of arguments
 * @param argv - arguments
 * @return - 0 success,1 error
 */
int main(int argc,char**argv) {
    struct hostent *server;
    struct sockaddr_in server_address;
    int port;
    char *mode;
    char *server_ip;
    arg_check(argc);
    arg_set(argv,argc,&mode,&server_ip,&port);
    address(&server,&server_ip);
    server_address = ip_server(server,port);
    if (strcmp(mode, "tcp") == 0) {
        create_socket(SOCK_STREAM);
        establish_connection(server_address);
        tcp_loop();
        close(client_socket);
        return 0;
    } else {
        create_socket(SOCK_DGRAM);
        udp_loop(server_address);
        return 0;
    }
}
