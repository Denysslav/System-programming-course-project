#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>

#include "server_const.h"
#include "server_types.h"
#include "server_func.h"
#include "server_util.h"

void admin();
void server();

int main() {

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    if (fork() == 0) {
        admin();
    } else {
        server();
    }

    return 0;
}

void admin() {
    initSysAdmin();

    while (1) {
        sysAdminListen();
    }
}

void server() {
    userContainer = (struct user *)malloc(sizeof(struct user) * USER_CONTAINER_INIT_CAPACITY);
    if (userContainer == NULL) {
        printf("Unable to allocate memory for users container. Exiting...");
        exit(1);
    }


    blacklist = (struct ip_track *)malloc(sizeof(struct ip_track) * BLACKLIST_INIT_CAPACITY);
    if (blacklist == NULL) {
        printf("Unable to allocate memory for blacklist. Exiting...");
        exit(1);
    }

    int connectionCount = 0,
        serverSocket, newConnSocket;

    pthread_t tid[SERVER_MAX_CONN];
    socklen_t addrSize;

    struct thread_args args;
    struct sockaddr_storage serverStorage;

    loadUsers(userContainer);
    loadBlacklist(blacklist);


    bootServer(&serverSocket);

    while (1) {

        addrSize = sizeof(serverStorage);
        newConnSocket = accept(serverSocket, (struct sockaddr *) &serverStorage, &addrSize);

        args.socketId = newConnSocket;
        args.client = serverStorage;

        if (pthread_create(&tid[connectionCount++], NULL, serve, &args) != 0) {
            --connectionCount;
            printf("Failed to create thread\n");
        }

        if (connectionCount >= SERVER_MAX_CONN) {
            int i = 0;
            for (i = 0; i < SERVER_MAX_CONN; i++) {
                pthread_join(tid[i], NULL);
            }

            connectionCount = 0;
        }
    }
}