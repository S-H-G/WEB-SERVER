#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/select.h>

#define BYTES 1024
#define MAX_PATH 1024
#define MAX_MSG 4096
#define MAX_LINE 8192

void server_init(char *);
void parser(int );
void webroot();
void check_clients();
void php_cgi(int , char *);

typedef struct {
        int maxfd;
        int nready;
        fd_set read_set;
        fd_set ready_set;
} pool;

char *path;
int listenfd;
pool p;
