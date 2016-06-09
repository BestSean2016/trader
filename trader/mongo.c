#include "mongo.h"

static MONGO_CONFIG mongocnf;

int mongo_init(const char *host, const short port, const char *dbname,
               const char *collection) {
  memset(&mongocnf, 0, sizeof(mongocnf));
  snprintf(mongocnf.url, sizeof(mongocnf.url), "mongodb://%s:%d", host, port);
  snprintf(mongocnf.dbname, sizeof(mongocnf.dbname), "%s", dbname);
  snprintf(mongocnf.collection, sizeof(mongocnf.collection), "%s", collection);

  mongoc_init();

  mongocnf.uri = mongoc_uri_new(mongocnf.url);
  if (!mongocnf.uri)
    return (-1);
  mongocnf.pool = mongoc_client_pool_new(mongocnf.uri);
  if (!mongocnf.pool) {
    mongoc_uri_destroy(mongocnf.uri);
    return (-2);
  }

  return (0);
}

void mongo_clean() {
  mongoc_client_pool_destroy(mongocnf.pool);
  mongoc_uri_destroy(mongocnf.uri);
  mongoc_cleanup();
}

MONGO *mongo_open() {
  MONGO *mongo = 0;
  if (mongocnf.pool) {
    mongo = (MONGO *)calloc(1, sizeof(MONGO));
    mongo->client = mongoc_client_pool_pop(mongocnf.pool);
    if (!mongo->client)
      goto bad_exit;
    mongo->collection = mongoc_client_get_collection(
        mongo->client, mongocnf.dbname, mongocnf.collection);
    if (!mongo->collection)
      goto bad_exit;
  }
  return mongo;
bad_exit:
  mongo_close(mongo);
  return 0;
}

void mongo_close(MONGO *mongo) {
    if (mongo) {
      if (mongo->collection)
        mongoc_collection_destroy(mongo->collection);
      if (mongo->client)
        mongoc_client_destroy(mongo->client);
      free(mongo);
    }
}
