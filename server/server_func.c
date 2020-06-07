#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "../common_const.h"
#include "server_func.h"
#include "server_util.h"
#include "server_const.h"
#include "server_types.h"

struct ip_track *blacklist = NULL;
struct user *userContainer = NULL;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int shmemId = 0;

void bootServer(int *serverSock) {
    int serverSocket;

    struct sockaddr_in serverAddress;

    serverSocket = socket(PF_INET, SOCK_STREAM, 0);

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    memset(serverAddress.sin_zero, '\0', sizeof(serverAddress.sin_zero));

    if (bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0 ) {
        printf("Error binding server to port\n");
        exit(1);
    }

    if (listen(serverSocket, 5) != 0) {
        printf("Error starting server\n");
        exit(1);
    }

    *serverSock = serverSocket;

    printf("Server listening on port %d\n", SERVER_PORT);
}

void *serve(void *arguments) {
    struct thread_args *args = (struct thread_args *)arguments;

    char clientIpAddress[INET_ADDRSTRLEN], client_message[1024];
    getClientIp(args->client, clientIpAddress);

    pthread_mutex_lock(&lock);

    if (isIpBlacklisted(blacklist, clientIpAddress)) {
        send(args->socketId, ERR_IP_BANNED, strlen(ERR_IP_BANNED), 0);

        goto releaseConnection;
    } else {
        send(args->socketId, CONNECTION_APPROVED, strlen(CONNECTION_APPROVED), 0);
    }

    recv(args->socketId, client_message, 1024, 0);

    struct user usr;
    if (decodeCredentials(client_message, &usr) != 0) {
        if (findUser(userContainer, usr) == 1) {
            send(args->socketId, CREDENTIALS_APPROVED, strlen(CREDENTIALS_APPROVED), 0);

            goto releaseConnection;
        }
    }

    send(args->socketId, ERR_INVALID_CREDENTIALS, strlen(ERR_INVALID_CREDENTIALS), 0);

    incrementBlacklistEntryCount(blacklist, clientIpAddress);

    releaseConnection:
        pthread_mutex_unlock(&lock);

        close(args->socketId);

        pthread_exit(NULL);
}


void signalHandler(int signo) {
    switch (signo) {
        case SIGINT:
        case SIGTERM:
            persistUsers(userContainer, containerElements);
            persistBlacklist(blacklist, blacklistElements);
            shmctl(shmemId, IPC_RMID, NULL);
            free(userContainer);
            free(blacklist);
            exit(0);
            break;
        default:
            break;
    }
}


void initSysAdmin() {
    key_t key = ftok("shmfile", 65);

    shmemId = shmget(key, 1024, 0666 | IPC_CREAT);
}

void sysAdminListen() {
    char *str = (char *) shmat(shmemId,(void*)0,0);
    char *buffer = strdup(str);

    if (strncmp(str, "register:", 9) == 0) {
        struct user newUser;

        decodeCredentials(buffer, &newUser);
        addUser(userContainer, newUser);

        printf("Sys admin said => New user was registered\n");
    } else if (strcmp(buffer, "") != 0){
        printf("Sys admin said => %s\n", buffer);
    }

    shmdt(str);
}
