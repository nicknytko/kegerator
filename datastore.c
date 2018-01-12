#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "datastore.h"

#define DATA_STORE_PATH "brew.db"

static struct list_t listHead;
static uint8_t numBrews = 0;

static void deleteListRecursively( struct list_t* node )
{
    if ( node != NULL )
    {
        free( node->data->name );
        free( node->data );
        deleteListRecursively( node->next );
        free( node );
    }
}

int dataStoreInit( )
{
    listHead.data = NULL;
    listHead.next = NULL;

    dataStoreLoad( );
    return 0;
}

void dataStoreQuit( )
{
    dataStoreSave( );
    deleteListRecursively( listHead.next );
}

void dataStoreSave( )
{
    FILE* brewdb = fopen( DATA_STORE_PATH, "wb" );    
    fwrite( &numBrews, sizeof( numBrews ), 1, brewdb );

    struct list_t* node = listHead.next;    
    for ( uint32_t i = 0; i < numBrews; i++ )
    {
        struct brewdata_t* data = node->data;
        uint32_t nameLength = strlen( data->name );

        fwrite( &nameLength, sizeof( uint32_t ), 1, brewdb );
        fwrite( data->name, sizeof( char ), nameLength, brewdb );
        fwrite( &( data->abv ), sizeof( uint32_t ), 1, brewdb );
        fwrite( &( data->mLRemaining ), sizeof( uint64_t ), 1, brewdb );
        node = node->next;
    }

    fclose( brewdb );
}

void dataStoreLoad( )
{
    FILE* brewdb = fopen( DATA_STORE_PATH, "rb" );
    if ( brewdb == NULL )
    {
        return;
    }
    
    fread( &numBrews, sizeof( numBrews ), 1, brewdb );
    for ( uint32_t i = 0; i < numBrews; i++ )
    {
        struct brewdata_t* data = (struct brewdata_t*)
            malloc( sizeof( struct brewdata_t ) );
        uint32_t nameLength;
        
        fread( &nameLength, sizeof( uint32_t ), 1, brewdb );
        data->name = (char*) calloc( nameLength + 1, sizeof( char ) );
        fread( data->name, sizeof( char ), nameLength, brewdb );
        data->name[nameLength] = '\0';

        fread( &( data->abv ), sizeof( uint32_t ), 1, brewdb );
        fread( &( data->mLRemaining ), sizeof( uint64_t ), 1, brewdb );

        dataStoreInsertElement( data );
    }

    fclose( brewdb );
}

void dataStoreInsertElement( struct brewdata_t* data )
{
    struct list_t* node = (struct list_t*) malloc( sizeof( struct list_t ) );
    node->data = data;
    node->next = listHead.next;
    listHead.next = node;
}

struct list_t* dataStoreGetBrews( )
{
    return listHead.next;
}
