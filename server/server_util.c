#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server_const.h"
#include "server_types.h"
#include "../common_const.h"

int containerElements = 0,
    maxContainerElements = 0,
    blacklistElements = 0,
    maxBlacklistElements = 0;

ssize_t readLine(char *buffer, char *filename, int size, off_t *offset) {
    int fd = open(filename, O_RDONLY);

    if (fd == -1) return -1;

    ssize_t charCount = 0, counter = 0;
    char *p = NULL;
    if ((charCount = lseek (fd, *offset, SEEK_SET)) != -1) {
        charCount = read (fd, buffer, size);
    }

    close(fd);
    if (charCount == -1) return -1;

    p = buffer;
    while (counter < charCount && *p != '\n') p++, counter++;

    *p = 0;
    if (counter == charCount) {
        *offset += charCount;

        return charCount < (ssize_t)size ? charCount : 0;
    }

    *offset += counter + 1;

    return counter;
}

int loadUsers(struct user *users) {
    int maxSize = USER_PASS_MAX_LENGTH + USER_NAME_MAX_LENGTH + 2;
    char line[maxSize];
    char *token = NULL;

    struct user data;

    containerElements = 0;
    maxContainerElements = USER_CONTAINER_INIT_CAPACITY;

    ssize_t len;
    off_t offset = 0;
    while ( (len = readLine(line, "users.txt", maxSize, &offset)) != -1) {
        if (len == 0) break;

        if (containerElements == maxContainerElements) {
            maxContainerElements += USER_CONTAINER_INIT_CAPACITY;
            users = realloc(users, maxContainerElements * sizeof(struct user));
        }

        token = strtok(line, "+");
        data.name = (char *)malloc(sizeof(token) + 1);
        data.name = strdup(token);

        token = strtok(NULL, "+");
        data.password = (char *)malloc(sizeof(token) + 1);
        data.password = strdup(token);

        users[containerElements++] = data;
    }

    return 0;
}

int loadBlacklist(struct ip_track *blacklist) {
    int maxSize = INET_ADDRSTRLEN + 18;

    char line[maxSize];
    char *token = NULL;

    struct ip_track data;

    blacklistElements = 0;
    maxBlacklistElements = BLACKLIST_INIT_CAPACITY;

    ssize_t len;
    off_t offset = 0;
    while ( (len = readLine(line, "blacklist.txt", maxSize, &offset)) != -1) {
        if (len == 0) break;

        if (blacklistElements == maxBlacklistElements) {
            maxBlacklistElements += BLACKLIST_INIT_CAPACITY;
            blacklist = realloc(blacklist, maxBlacklistElements * sizeof(struct ip_track));
        }

        token = strtok(line, "+");
        strcpy(data.ipAddress, token);

        token = strtok(NULL, "+");
        data.attempts = strtol(token, NULL, 10);

        blacklist[blacklistElements++] = data;
    }

    return 0;
}

int isIpBlacklisted(struct ip_track *blacklisters, char *ip) {
    int i = 0;
    for (i = 0; i < blacklistElements; i++) {
        if (strcmp(blacklisters[i].ipAddress, ip) == 0
            && blacklisters[i].attempts >= BLACKLIST_ATTEMPTS_THRESHOLD) {
            return 1;
        }
    }

    return 0;
}

void getClientIp(struct sockaddr_storage clientSockAddrIn, char *ip) {
    struct sockaddr_in* clientSockAddr = (struct sockaddr_in *)&clientSockAddrIn;
    struct in_addr clientInAddr = clientSockAddr->sin_addr;

    inet_ntop(AF_INET, &clientInAddr, ip, INET_ADDRSTRLEN);
}

int addUser( struct user* users, struct user usr) {
    int pos = containerElements;

    if (pos >= maxContainerElements) {
        maxContainerElements += USER_CONTAINER_INIT_CAPACITY;
        users = realloc(users, maxContainerElements * sizeof(struct user));
    }

    users[pos] = usr;

    blacklistElements += 1;
    return 1;
}


int findUser(struct user *users, struct user usr) {
    int i = 0;
    for (i = 0; i < containerElements; i++) {
        if (strcmp(users[i].name, usr.name) == 0 &&
            strcmp(users[i].password, usr.password) == 0) {
            return 1;
        }
    }

    return 0;
}

int decodeCredentials(char *request, struct user *usr) {
    char *token = strtok(request, "+");
    if (token == NULL) {
        return 0;
    }

    usr->name = (char *)malloc(sizeof(token) + 1);
    usr->name = strdup(token);

    token = strtok(NULL, "+");
    if (token == NULL) {
        return 0;
    }

    usr->password = (char *)malloc(sizeof(token) + 1);
    usr->password = strdup(token);

    return 1;
}

int incrementBlacklistEntryCount(struct ip_track *blacklisters, char *ip) {
    int pos = blacklistElements, i = 0;
    for (i = 0; i < blacklistElements; i++) {
        if (strcmp(blacklisters[i].ipAddress, ip) == 0) {
            blacklisters[i].attempts += 1;

            return 1;
        }
    }

    struct ip_track data;
    if (pos >= maxBlacklistElements) {
        maxBlacklistElements += BLACKLIST_INIT_CAPACITY;
        blacklisters = realloc(blacklisters, maxBlacklistElements * sizeof(struct ip_track));
    }

    strcpy(data.ipAddress, ip);
    data.attempts = 1;

    blacklisters[pos] = data;

    blacklistElements += 1;
    return 1;
}

int persistUsers(struct user *users, int listSize) {
    int fd = open("users.txt", O_WRONLY);
    if (fd == -1) {
        return -1;
    }

    char newLine = '\n', separator = '+';
    int i = 0;
    for (i = 0; i < listSize; i++) {
        write(fd, users[i].name, strlen(users[i].name));
        write(fd, &separator, sizeof(separator));
        write(fd, users[i].password, strlen(users[i].password));
        write(fd, &newLine, sizeof(newLine));
    }

    close(fd);

    return 1;
}

int persistBlacklist(struct ip_track *blacklisters, int listSize) {
    int fd = open("blacklist.txt", O_CREAT | O_WRONLY);
    if (fd == -1) {
        return -1;
    }

    char newLine = '\n';
    int i = 0;
    for (i = 0; i < listSize; i++) {
        write(fd, blacklisters[i].ipAddress, strlen(blacklisters[i].ipAddress));
        write(fd, &blacklisters[i].attempts, sizeof(blacklisters[i].attempts));
        write(fd, &newLine, sizeof(newLine));
    }

    close(fd);

    return 1;
}