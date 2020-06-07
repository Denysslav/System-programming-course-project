#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "../common_const.h"

int makeServerConnection(int *sockId);
void getUsername(char *name);
void getPassword(char *pass);
void printServerMessage(char *code);
void doLogin(int socketId);
void doRegister();

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Invalid number of CL arguments\n");
        exit(1);
    }

    int clientSocket;
    char response[4];

    if (makeServerConnection(&clientSocket) != 0) {
        printf("Failed to establish connection with server. Exiting...");
        exit(1);
    }

    if (recv(clientSocket, response, 4, 0) < 0) {
        printf("Socket receive failed\n");
    }

    printServerMessage(response);

    if (strcmp(argv[1], "login") == 0) {
        doLogin(clientSocket);
    } else if (strcmp(argv[1], "register") == 0) {
        doRegister();
    } else {
        printf("Invalid operation\n");
    }

    close(clientSocket);
}

int makeServerConnection(int *sockId) {
    struct sockaddr_in serverAddress;

    socklen_t addressSize;

    *sockId = socket(PF_INET, SOCK_STREAM, 0);

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    memset(serverAddress.sin_zero, '\0', sizeof serverAddress.sin_zero);

    addressSize = sizeof(serverAddress);

    return connect(*sockId, (struct sockaddr *) &serverAddress, addressSize);
}

void doLogin(int socketId) {
    char username[USER_NAME_MAX_LENGTH],
        password[USER_PASS_MAX_LENGTH],
        request[USER_NAME_MAX_LENGTH + USER_PASS_MAX_LENGTH + 2],
        response[4];

    getUsername(username);
    getPassword(password);

    strcpy(request, username);
    strcat(request, "+");
    strcat(request, password);

    if (send(socketId, request, strlen(request), 0) < 0) {
        printf("Socket send failed\n");
    }

    if (recv(socketId, response, 4, 0) < 0) {
        printf("Socket receive failed\n");
    }

    printServerMessage(response);
}

void doRegister() {
    char username[USER_NAME_MAX_LENGTH],
         password[USER_PASS_MAX_LENGTH],
         payload[USER_NAME_MAX_LENGTH + USER_PASS_MAX_LENGTH + 15];

    key_t key = ftok("shmfile",65);

    int shmid = shmget(key, 1024, 0666 | IPC_CREAT);

    char *str = (char*) shmat(shmid,(void*)0,0);

    getUsername(username);
    getPassword(password);

    strcpy(payload, "register:");
    strcat(payload, username);
    strcat(payload, "+");
    strcat(payload, password);

    strcpy(str, payload);

    shmdt(str);
}

void getUsername(char *name) {
    char buffer[USER_NAME_MAX_LENGTH + 1];

    write(STDOUT_FILENO, "Enter username: ", 16);
    read(STDIN_FILENO, buffer, USER_NAME_MAX_LENGTH);

    strncpy(name, buffer, strlen(buffer) - 1);
    strcat(name, "\0");
}

void getPassword(char *pass)  {
    char buffer[USER_PASS_MAX_LENGTH + 1];

    write(STDOUT_FILENO, "Enter password: ", 16);
    read(STDIN_FILENO, buffer, USER_PASS_MAX_LENGTH);

    strncpy(pass, buffer, strlen(buffer) - 1);
    strcat(pass, "\0");
}


void printServerMessage(char *code) {
    if (strcmp(code, CONNECTION_APPROVED) == 0) {

    } else if (strcmp(code, CREDENTIALS_APPROVED) == 0) {
        printf("Successful login\n");
    } else if (strcmp(code, ERR_IP_BANNED) == 0) {
        printf("Your IP address is blocked!\n");
        exit(1);
    } else if (strcmp(code, ERR_INVALID_CREDENTIALS) == 0) {
        printf("Invalid login credentials!\n");
    } else {
        printf("Unknown server response code: %s\n", code);
    }
}