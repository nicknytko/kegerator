#ifndef DATASTORE_H_
#define DATASTORE_H_

#include <stdint.h>

struct brewdata_t
{
    char* name;
    uint32_t abv;
    uint64_t mLRemaining;
};

struct list_t
{
    struct brewdata_t* data;
    struct list_t* next;
};

int dataStoreInit( );
void dataStoreQuit( );
void dataStoreSave( );
void dataStoreLoad( );
void dataStoreInsertElement( struct brewdata_t* data );
struct list_t* dataStoreGetBrews( );

#endif
