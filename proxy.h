#include "mycache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define HOSTNAME_MAX_LEN 63
#define PORT_MAX_LEN 10
#define HEADER_NAME_MAX_LEN 32
#define HEADER_VALUE_MAX_LEN 64
#define METHOD_MAX_LEN 10


/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";


typedef struct {
    char name[HEADER_NAME_MAX_LEN];
    char value[HEADER_VALUE_MAX_LEN];
} ReqHeader;



void *thread(void *vargp);
void doit(int fd);
void parse_request(int fd, ReqLine *rql, ReqHeader *hdrs, int *num_hdrs);
void parse_uri(char *uri, ReqLine *rql);
int parse_hdrs(rio_t *riop, ReqHeader *hdrs, char *buf);
int forward_request(ReqLine *rql, ReqHeader *hdrs, int num_hdrs);
void *test_thread(void *vargp);

int requestLine_copy(ReqLine *des, ReqLine *source) {
    strcpy(des->host, source->host);
    strcpy(des->port, source->port);
    strcpy(des->path, source->path);
    return 1;
}