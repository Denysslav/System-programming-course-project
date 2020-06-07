#ifndef SPR_COURSE_PROJ_SERVER_FUNC_H
#define SPR_COURSE_PROJ_SERVER_FUNC_H

#include "server_types.h"

extern struct ip_track *blacklist;
extern struct user *userContainer;

extern pthread_mutex_t lock;
extern int shmemId;

void bootServer(int *serverSock);
void *serve(void *arguments);
void signalHandler(int signo);
void initSysAdmin();
void sysAdminListen();

#endif //SPR_COURSE_PROJ_SERVER_FUNC_H
