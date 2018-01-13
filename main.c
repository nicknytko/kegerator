#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "server.h"
#include "flowsensor.h"
#include "datastore.h"
#include "daemon_ipc.h"

#define DAEMON_SOCKET_FILE "./kegerator_socket"

void signalHandler( int signal )
{
    serverQuit( );
}

void printUsage( const char* exec )
{
    printf( "usage: \n" );
    printf( "\t%s --help\n", exec );
    printf( "\t%s --stop\n", exec );
    printf( "\t%s --addbrew [name] [abv] [mL left]\n", exec );
    printf( "\t%s --listbrews\n", exec );
}

void client( int argc, char** argv, int socketFile, struct sockaddr_un* sock )
{
    enum daemon_ipc_type ipcType;

    if ( argc == 1 || strcmp( argv[1], "--help" ) == 0 || strcmp( argv[1], "-h" ) == 0 )
    {
        printUsage( argv[0] );
        return;
    }
    
    if ( argc > 1 )
    {
        if ( strcmp( argv[1], "--stop" ) == 0 )
        {
            ipcType = DAEMON_QUIT;
            write( socketFile, &ipcType, sizeof( enum daemon_ipc_type ) );
            return;
        }
        if ( strcmp( argv[1], "--addbrew" ) == 0 &&
             argc == 5 )
        {
            struct daemon_ipc_brew_data_t newBrew;
            uint32_t abvWhole, abvDec;
            strncpy( newBrew.brewName, argv[2], 128 );
            sscanf( argv[3], "%u.%u", &abvWhole, &abvDec );
            sscanf( argv[4], "%llu", &( newBrew.mLRemaining ) );
            
            while ( abvDec > 10 )
            {
                abvDec /= 10;
            }
            newBrew.abv = abvWhole * 10 + abvDec;

            ipcType = DAEMON_ADD_BREW;
            write( socketFile, &ipcType, sizeof( enum daemon_ipc_type ) );
            write( socketFile, &newBrew, sizeof( struct daemon_ipc_brew_data_t ) );
            return;
        }
        if ( strcmp( argv[1], "--listbrews" ) == 0 &&
             argc == 2 )
        {
            uint32_t numBrews;
            struct daemon_ipc_brew_data_t* brewData;

            ipcType = DAEMON_LIST_BREWS;
            write( socketFile, &ipcType, sizeof( enum daemon_ipc_type ) );

            read( socketFile, &numBrews, sizeof( uint32_t ) );
            brewData = calloc( numBrews, sizeof( struct daemon_ipc_brew_data_t ) );
            read( socketFile, brewData, sizeof( struct daemon_ipc_brew_data_t ) * numBrews );
            
            for ( uint32_t i = 0; i < numBrews; i++ )
            {
                printf( "%-32.32s\t%u.%u%%\t%llu mL\n", brewData[i].brewName,
                        brewData[i].abv / 10, brewData[i].abv % 10,
                        brewData[i].mLRemaining );
            }
            
            free( brewData );
            return;
        }
    }
    printUsage( argv[0] );
}

int main( int argc, char** argv )
{
    /* Determine if running as daemon or client */

    struct sockaddr_un socketAddr;
    int socketFile = socket( AF_UNIX, SOCK_STREAM, 0 );
    socketAddr.sun_family = AF_UNIX;
    strcpy( socketAddr.sun_path, DAEMON_SOCKET_FILE );

    /* Try to connect as client */
    
    if ( connect( socketFile, (struct sockaddr*) &socketAddr,
                  sizeof( socketAddr ) ) == 0 )
    {
        client( argc, argv, socketFile, &socketAddr );
        return 0;
    }

    /* We couldn't connect, so set up socket server for IPC */

    bind( socketFile, (struct sockaddr*) &socketAddr, sizeof( socketAddr ) );
    listen( socketFile, 10 );
    chmod( DAEMON_SOCKET_FILE, 0777 );
    fcntl( socketFile, F_SETFL, fcntl( socketFile, F_GETFL, 0 ) | O_NONBLOCK );
    
    /* Fork self, detach and run program as a daemon */

    pid_t pid = fork( );
    if ( pid > 0 )
    {
        exit( 0 );
    }
    if ( flowSensorInit( ) != 0 )
    {
        return 1;
    }
    if ( dataStoreInit( ) != 0 )
    {
        return 2;
    }
    signal( SIGTERM, signalHandler );    

    printf( "Starting kegerator daemon ...\n" );
    serverMain( socketFile );
    printf( "Closing kegerator daemon...\n" );
    
    close( socketFile );
    unlink( DAEMON_SOCKET_FILE );
    dataStoreQuit( );
    flowSensorQuit( );
    
    return 0;
}
