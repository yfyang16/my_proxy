#include "mycache.h"
#include "csapp.h"

// WebCache myCache;
// int writer_exist = 0;

// convert list entry to page
#define le2obj(le, member)                 \
    to_struct((le), HtmlObject, member)

// #define head (myCache.head)

/*
 * Find if there is a cache in the proxy. If so, write the cache
 * into the fd. If not, we check the size and put it into 
 * our cache when there is enough space.
 */
int find_cache(char *method, ReqLine *rql, int fd, WebCache *myCachep) {
	printf("In the find_cache function\n");

    list_entry_t *le = list_next(&(myCachep->head));

    while (le != &(myCachep->head)) {
        HtmlObject *obj = le2obj(le, object_link);

        if (strcmp(method, obj->method)) {continue;}

        if (!request_cmp(&obj->rql, rql)) {
        	write_into_fd(obj, fd);
        	return 1;
        }
        le = list_next(le);
     
    }

    // If not in cache
    return 0;
}

/*
 *  If rql1 == rql2, return 0 else return 1
 */
int request_cmp(ReqLine *rql1, ReqLine *rql2) {
	if (strcmp(rql1->host, rql2->host)) return 1;
	if (strcmp(rql1->port, rql2->port)) return 1;

	if (strcmp(rql1->path, rql2->path)) return 1;
	else {return 0;}
}


int write_into_fd(HtmlObject *obj, int fd) {
	Rio_writen(fd, obj->object, obj->size);
	return 1;
}



int write_into_cache(int obj_size, char *obuf, char *method, ReqLine *rql, WebCache *myCachep) {
	printf("In the write_into_cache function\n");
	if (obj_size > MAX_OBJECT_SIZE) return -1;
	HtmlObject *obj;
	rio_t rio;
	char *contentp;
	int line_size;
	char buf[MAXLINE];

	if ((obj_size + myCachep->size) > MAX_CACHE_SIZE) {
		P(&(myCachep->sem));
	    myCachep->writer_exist = 1;
	    list_entry_t *le = list_prev(&(myCachep->head));
	    obj = le2obj(le, object_link);
	    list_del_init(le);
	    myCachep->size -= obj->size;
	    free(obj);
	    myCachep->writer_exist = 0;
	    V(&(myCachep->sem));
		return -2;

	} else {
		obj = Malloc(sizeof(HtmlObject));
		list_init(&obj->object_link);
		strcpy(obj->method, method);
		printf("After copy method: %s\n", obj->method);
		requestLine_copy(&obj->rql, rql);
		printf("After copy rql\n");
		obj->size = obj_size;
		//contentp = obj->object;

		//Rio_readinitb(&rio, connfd);
	    //while(line_size = Rio_readlineb(&rio, buf, MAXLINE)) {
	    //    strcpy(contentp, buf);
	    //    contentp += line_size;
	    //}
        strcpy(obj->object, obuf);
	    printf("obj_size: %d\n", obj_size);
        printf("OBJ Content:\n%s\n", obj->object);
	    P(&(myCachep->sem));
	    myCachep->writer_exist = 1;
	    list_add_before(&(myCachep->head), &obj->object_link);
	    myCachep->size += obj_size;
	    myCachep->writer_exist = 0;
	    V(&(myCachep->sem));

	    return 1;
	}
}






