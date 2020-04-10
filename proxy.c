/*
 *   Author: Yufeng Yang
 *   Time: 04/2020
 *   
 *   This lab is to construct an proxy which could handle 
 *   multiple requests and cache web objects.
 */
#include <stdio.h>
#include "proxy.h"

WebCache myCache;
int main(int argc, char **argv)
{

    int listenfd, *connfdp, rs;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t *tidp;

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    list_init(&myCache.head);
    myCache.writer_exist = 0;
    rs = sem_init(&myCache.sem, 0, 1);
    printf("semaphore created: %d\n", rs);


    listenfd = Open_listenfd(argv[1]);
    while (1) {
        tidp = Malloc(sizeof(pthread_t));
        connfdp = Malloc(sizeof(int));
        clientlen = sizeof(clientaddr);
        *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen); 
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        pthread_create(tidp, NULL, thread, connfdp);  
    }
}


/*
 *  The request handler will create a new thread for every request.
 *  It will forward the request to the server or directly return the
 *  response to the client.
 */
void *thread(void *vargp) {
    int connfd = *((int*)vargp);
    // printf("Successfully get the connfd: %d\n", connfd);

    // detach this thread in order to let itself reclaim the resources
    Pthread_detach(pthread_self());

    // After we copy the info from the pointer vargp to the local connfd descriptor,
    // we free the memory pointed by vargp
    Free(vargp);

    doit(connfd);

    // close this file descriptor
    Close(connfd);
    // printf("=============End of one user================\n\n");
}

/*
 * For each request from clients, we do the following things.
 * (1) parse the request and check if it is valid
 * (2) If so, the handler will connect the server and forward
 *     the response to the connfd.
 */
void doit(int fd) {
    ReqLine request_line;
    ReqHeader headers[20];
    int num_hdrs, line_size, connfd;
    int cache_exist = 0, obj_size = 0;
    rio_t rio;
    char buf[MAXLINE];

    // Parse the request and check if it is valid
    parse_request(fd, &request_line, headers, &num_hdrs);


    // If in cache, directly copy the response to fd and return
    while (myCache.writer_exist == 1);
    cache_exist = find_cache("GET", &request_line, fd, &myCache);
    

    if (!cache_exist) {
	    // If not in cache, forward the request to server
	    connfd = forward_request(&request_line, headers, num_hdrs);
	    Rio_readinitb(&rio, connfd);
	    while(line_size = Rio_readlineb(&rio, buf, MAXLINE)) {
	        Rio_writen(fd, buf, line_size);
	        obj_size += line_size;
	    }

	    write_into_cache(obj_size, connfd, "GET", &request_line, &myCache);
	}

    Close(connfd);


}

void parse_request(int fd, ReqLine *rql, ReqHeader *hdrs, int *num_hdrs) {
    // printf("In the parse_request function\n");
    rio_t rio;
    int rn = 0;
    char method[10], uri[MAXLINE], version[32], buf[MAXLINE];

    Rio_readinitb(&rio, fd);
    rn = Rio_readlineb(&rio, buf, MAXLINE);

    if (rn == 0) {
        return;
    } else if (rn == -1) {
        fprintf(stderr, "Error when reading the request from fd!");
        exit(0);
    }
    buf[rn+1] = '\0';
    // printf("Get the request line\n");
    printf("See the buf: %s\n", buf);
    // parse request line (GET http://www.abcd.com[:80]/path_str HTTP/1.1)
    sscanf(buf, "%s %s %s", method, uri, version);
    printf("Request Line:%s %s %s\n", method, uri, version);
    parse_uri(uri, rql);

    // parse request headers (header_name: header data\r\n)
    *num_hdrs = parse_hdrs(&rio, hdrs, buf);
}

void parse_uri(char *uri, ReqLine *rql) {
    char c;
    int i = 0, pad=0;
    printf("URI string: %s\n", uri);

    // compare "http://"
    const char* http_str = "http://";
    for (i = 0;i < 7;i++) {
        if (uri[i] != http_str[i]) {
            fprintf(stderr, "Error: invalid uri!\n");
            exit(0);
        }
        rql->host[i] = http_str[i];
    }

    // copy host_name
    c = uri[i];
    while ((c != ':') && (c != '/')) {
        rql->host[i-7] = c;
        i ++;
        c = uri[i];
    }
    rql->host[i-7] = '\0';
    printf("Request Line Host Name: %s\n", rql->host);

    // check if it has a port
    pad = i;
    
    if (uri[i] == ':'){
        i++;
        pad = i;
        c = uri[i];
        while ((c != '/')) {
            rql->port[i - pad] = c;
            i ++;
            c = uri[i];
        }
    }
    rql->port[i-pad] = '\0';
    printf("Request Line Port (If have): %s\n", rql->port);

    // copy path
    uri += i;
    strcpy(rql->path, uri);
    printf("Request Line Path: %s\n", rql->path);
}

int parse_hdrs(rio_t *riop, ReqHeader *hdrs, char *buf) {
    // printf("In parse_hdrs function\n");
    int i;
    int lineIndex = 0;

    Rio_readlineb(riop, buf, MAXLINE);

    while (strcmp(buf, "\r\n")) {
        i = 0;
        while (buf[i] != ':') {
            hdrs[lineIndex].name[i] = buf[i];
            i ++;
        }
        i += 2;
        while (buf[i] != '\r') {
            hdrs[lineIndex].value[i] = buf[i];
            i ++;
        }
        lineIndex ++;
        Rio_readlineb(riop, buf, MAXLINE);
    }

    return lineIndex;
}

int forward_request(ReqLine *rql, ReqHeader *hdrs, int num_hdrs) {
    rio_t rio;
    char buf[MAXLINE], *bp = buf;
    int req_size, tmp_len, clientfd;
    short host_exist = 0, userAgent_exist = 0, conn_exist = 0, pconn_exist = 0;

    // printf("Before open fd: host(%s), port(%s)\n", rql->host,rql->port);
    clientfd = Open_clientfd(rql->host, rql->port);
    Rio_readinitb(&rio, clientfd);

    sprintf(bp, "GET %s HTTP/1.0\r\n\0", rql->path);
    tmp_len = strlen(buf);
    bp += tmp_len;
    req_size += tmp_len;


    // Check if "Host", "User-Agent", "Connection" and "Proxy-Connection" exists
    for (int i=0;i<num_hdrs;i++) {
        if (!strcmp(hdrs[i].name, "Host")) {
            host_exist = 1;
        }

        if (!strcmp(hdrs[i].name, "User-Agent")) {
            strcpy(hdrs[i].value, user_agent_hdr);
            userAgent_exist = 1;
        }

        if (!strcmp(hdrs[i].name, "Connection")) {
            sprintf(hdrs[i].value, "close");
            conn_exist = 1;
        }

        if (!strcmp(hdrs[i].name, "Proxy-Connection")) {
            sprintf(hdrs[i].value, "close");
            pconn_exist = 1;
        }
    }
    if (host_exist == 0) {
        sprintf(hdrs[num_hdrs].name, "Host");
        strcpy(hdrs[num_hdrs].value, rql->host);
        num_hdrs ++;
    }
    if (userAgent_exist == 0) {
        sprintf(hdrs[num_hdrs].name, "User-Agent");
        strcpy(hdrs[num_hdrs].value, user_agent_hdr);
        num_hdrs ++;
    }
    for (int i=0;i<num_hdrs;i++) {

        sprintf(bp, "%s: %s\r\n", hdrs[i].name, hdrs[i].value);
        tmp_len = (strlen(hdrs[i].name) + strlen(hdrs[i].value) + 4);
        bp += tmp_len;
        req_size += tmp_len;
    }


    if (conn_exist == 0) {
        sprintf(bp, "Connection: close\r\n");
        bp += 17;
        req_size += 17;
    }

    if (pconn_exist == 0) {
        sprintf(bp, "Proxy-Connection: close\r\n");
        bp += 23;
        req_size += 23;
    }

    sprintf(bp, "\r\n");
    req_size += 2;

    Rio_writen(clientfd, buf, req_size);
    return clientfd;

}







