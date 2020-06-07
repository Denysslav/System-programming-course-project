#ifndef SPR_COURSE_PROJ_SERVER_TYPES_H
#define SPR_COURSE_PROJ_SERVER_TYPES_H

#include <netinet/in.h>

struct user {
    char *name;
    char *password;
};

struct ip_track {
    char ipAddress[INET_ADDRSTRLEN];
    unsigned long attempts;
};

struct thread_args {
    int socketId;
    struct sockaddr_storage client;
};

#endif //SPR_COURSE_PROJ_SERVER_TYPES_H
