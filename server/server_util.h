#ifndef SPR_COURSE_PROJ_SERVER_UTIL_H
#define SPR_COURSE_PROJ_SERVER_UTIL_H

extern int containerElements,
        maxContainerElements,
        blacklistElements,
        maxBlacklistElements;

int loadUsers(struct user *users);
int loadBlacklist(struct ip_track *blacklist);
ssize_t readLine(char *buffer, char *filename, int size, off_t *offset);
int isIpBlacklisted(struct ip_track *blacklisters, char *ip);
void getClientIp(struct sockaddr_storage clientSockAddrIn, char *ip);
int findUser(struct user *users, struct user usr);
int addUser( struct user* users, struct user usr);
int decodeCredentials(char *request, struct user *usr);
int incrementBlacklistEntryCount(struct ip_track *blacklisters, char *ip);
int persistUsers(struct user *users, int listSize);
int persistBlacklist(struct ip_track *blacklisters, int listSize);

#endif //SPR_COURSE_PROJ_SERVER_UTIL_H
