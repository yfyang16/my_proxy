/*
 *   Author: Yufeng Yang
 *   Time: 04/2020
 *   
 *   The proxy will firstly look for the obj in my cache.
 *   The algorithm of this cache is temporarily FIFO and 
 *   we will finally use LRU algorithm.
 */
#include "mycache.h"
#include "csapp.h"

// WebCache myCache;

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

    P(&(myCachep->queue_sem));
        P(&(myCachep->rdcnt_sem));
            if (myCachep->rdcnt == 0) P(&(myCachep->sem));
            myCachep->rdcnt ++;
        V(&(myCachep->rdcnt_sem));
    V(&(myCachep->queue_sem));

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

    P(&(myCachep->rdcnt_sem));
        myCachep->rdcnt --;
        if (myCachep->rdcnt == 0) V(&(myCachep->sem));
    V(&(myCachep->rdcnt_sem));


    // If not in cache
    return 0;
}

/* If rql1 == rql2, return 0 else return 1 */
int request_cmp(ReqLine *rql1, ReqLine *rql2) {
    if (strcmp(rql1->host, rql2->host)) return 1;
    if (strcmp(rql1->port, rql2->port)) return 1;

    if (strcmp(rql1->path, rql2->path)) return 1;
    else {return 0;}
}


/* copy the html content in the "obj" to the fd */
int write_into_fd(HtmlObject *obj, int fd) {
    Rio_writen(fd, obj->object, obj->size);
    return 1;
}


/*
 * Put the html content in "obuf" into the WebCache
 * (1) Check the size
 * (2) Initializa a new HtmlObject and malloc some space 
 */
int write_into_cache(int obj_size, char *obuf, char *method, ReqLine *rql, WebCache *myCachep) {
    printf("In the write_into_cache function\n");
    if (obj_size > MAX_OBJECT_SIZE) return -1;
    HtmlObject *obj;
    rio_t rio;
    char *contentp;
    int line_size;
    char buf[MAXLINE];

    // If the cache is full, we have to replace one of the object
    // Else we initialize a new object for the incoming HtmlObject
    if ((obj_size + myCachep->size) > MAX_CACHE_SIZE) {

        // There is only on thread who can operate on our cache
        // which is a double-linked list.
        P(&(myCachep->queue_sem));
            P(&(myCachep->sem));
        V(&(myCachep->queue_sem));

        list_entry_t *le = list_prev(&(myCachep->head));
        obj = le2obj(le, object_link);
        list_del_init(le);
        myCachep->size -= obj->size;
        free(obj);

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

        strcpy(obj->object, obuf);
        printf("obj_size: %d\n", obj_size);
        printf("OBJ Content:\n%s\n", obj->object);

        P(&(myCachep->queue_sem));
            P(&(myCachep->sem));
        V(&(myCachep->queue_sem));

        list_add_before(&(myCachep->head), &obj->object_link);
        myCachep->size += obj_size;

        V(&(myCachep->sem));

        return 1;
    }
}






