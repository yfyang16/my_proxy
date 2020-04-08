/*
 *   Author: Yufeng Yang
 *   Time: 04/2020
 *   
 *   This lab is to construct an proxy which could handle 
 *   multiple requests and cache web objects.
 */

#include "proxy.h"

int main(int argc, char **argv)
{
    // printf("%s", user_agent_hdr);
    // return 0;

    int listenfd, *connfdp;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t *tidp;

    /* Check command line args */
    if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
    }
    // pthread_create(&tid, NULL, test_thread, NULL);
    // pthread_detach(tid);
    // pthread_join(tid, NULL);

    listenfd = Open_listenfd(argv[1]);
    while (1) {
    	tidp = Malloc(sizeof(pthread_t));
    	connfdp = Malloc(sizeof(int));
		clientlen = sizeof(clientaddr);
		*connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen); 
	    Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
	    printf("Accepted connection from (%s, %s)\n", hostname, port);
		pthread_create(tidp, NULL, thread, connfdp);  
		pthread_detach(*tidp);
		// pthread_join(tid, NULL);
		printf("After creating threads\n");                       
    }
}

void *test_thread(void *vargp) {
	// Pthread_detach(pthread_self());
	printf("We are good\n");
}


/*
 *  The request handler will create a new thread for every request.
 *  It will forward the request to the server or directly return the
 *  response to the client.
 */
void *thread(void *vargp) {
	printf("Successfully create a new thread\n");
	// Pthread_detach(pthread_self());
	int connfd = *((int*)vargp);
	printf("Successfully get the connfd: %d\n", connfd);
	// detach this thread in order to let itself reclaim the resources
	// Pthread_detach(pthread_self());

	// After we copy the info from the pointer vargp to the local connfd descriptor,
	// we free the memory pointed by vargp
	// Free(vargp);

    doit(connfd);

    // close this file descriptor
    Close(connfd);
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
    rio_t rio;
    char buf[MAXLINE];

	// Parse the request and check if it is valid
	parse_request(fd, &request_line, headers, &num_hdrs);


	// If in cache, directly copy the response to fd and return
	// TODO:


	// If not in cache, forward the request to server
	connfd = forward_request(&request_line, headers, num_hdrs);
	Rio_readinitb(&rio, connfd);
	while(line_size = Rio_readlineb(&rio, buf, MAXLINE)) {
		Rio_writen(fd, buf, line_size);
	}

	Close(connfd);


}

void parse_request(int fd, ReqLine *rql, ReqHeader *hdrs, int *num_hdrs) {
	rio_t rio;
	int rn;
	char method[10], uri[MAXLINE], version[32], buf[MAXLINE];

    Rio_readinitb(&rio, fd);
    rn = Rio_readlineb(&rio, buf, MAXLINE);

    if (rn == 0) {
        return;
    } else if (rn == -1) {
    	fprintf(stderr, "Error when reading the request from fd!");
    	exit(0);
    }

    //parse request line (GET http://www.abcd.com[:80]/path_str HTTP/1.1)
    sscanf(buf, "%s %s %s", method, uri, version);
    parse_uri(uri, rql);

    // parse request headers (header_name: header data\r\n)
    *num_hdrs = parse_hdrs(&rio, hdrs, buf);
}

void parse_uri(char *uri, ReqLine *rql) {
	char c;
	int i = 0, host_len, pad;

	// compare "http://"
	const char* http_str = "http://";
	for (i = 0;i < 7;i++) {
		if (uri[i] != http_str[i]) {
			fprintf(stderr, "Error: invalid uri!\n");
        	exit(0);
		}

	}

	// copy host_name
	while ((c != ':') && (c != '/')) {
		rql->host[i-7] = uri[i];
		i ++;
	}
	rql->host[i-7] = '\0';

	// check if it has a port
	pad = i;
	if (uri[i] == ':'){
		while ((c != '/')) {
			rql->port[i - pad] = uri[i];
			i ++;
		}
	}

	// copy path
	uri += i;
	strcpy(rql->path, uri);
}

int parse_hdrs(rio_t *riop, ReqHeader *hdrs, char *buf) {
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

	clientfd = Open_clientfd(rql->host, rql->port);
	Rio_readinitb(&rio, clientfd);

	sprintf(bp, "GET %s HTTP/1.0\r\n\0", rql->path);
	tmp_len = strlen(buf);
	bp += tmp_len;
	req_size += tmp_len;

	for (int i=0;i<num_hdrs;i++) {

		sprintf(bp, "%s: %s\r\n", hdrs[i].name, hdrs[i].value);
		tmp_len = (strlen(hdrs[i].name) + strlen(hdrs[i].value) + 4);
		bp += tmp_len;
		req_size += tmp_len;
	}

	sprintf(bp, "\r\n");
	req_size += 2;

	Rio_writen(clientfd, buf, req_size);
	return clientfd;

}







