#include "list.h"
#include "csapp.h"


/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define HOSTNAME_MAX_LEN 63
#define PORT_MAX_LEN 10
#define METHOD_MAX_LEN 10


typedef struct {
    int size;
    list_entry_t head;
    sem_t sem;
    int writer_exist;
} WebCache;

typedef struct {
	char host[HOSTNAME_MAX_LEN];
    char port[PORT_MAX_LEN];
    char path[MAXLINE];
} ReqLine;

typedef struct {
    char method[METHOD_MAX_LEN];
    ReqLine rql;
    char object[MAX_OBJECT_SIZE];
    list_entry_t object_link;
    int size;
} HtmlObject;

int find_cache(char *method, ReqLine *rql, int fd, WebCache *myCachep);
int request_cmp(ReqLine *rql1, ReqLine *rql2);
int write_into_cache(int obj_size, int connfd, char *method, ReqLine *rql, WebCache *myCachep);
int write_into_fd(HtmlObject *obj, int fd);