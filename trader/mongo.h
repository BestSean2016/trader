#ifndef MONGO_H
#define MONGO_H

#include <mongoc.h>

#define MONGO_URL_MAX_LEN 128
#define MONGO_MAX_NAME_LEN 64

typedef struct MONGO_CONFIG {
  char url[MONGO_URL_MAX_LEN];
  char dbname[MONGO_MAX_NAME_LEN];
  char collection[MONGO_MAX_NAME_LEN];
  mongoc_client_pool_t *pool;
  mongoc_uri_t *uri;
} MONGO_CONFIG;

typedef struct MONGO {
  mongoc_client_t *client;
  mongoc_collection_t *collection;

} MONGO;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern int mongo_init(const char *host, const short port, const char *dbname,
                      const char *collection);
extern void mongo_clean();
extern MONGO* mongo_open();
extern void mongo_close(MONGO* mongo);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // MONGO_H
