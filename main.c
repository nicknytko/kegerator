#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#include "server.h"
#include "flowsensor.h"
#include "datastore.h"

#define DAEMON_SOCKET_FILE "./kegerator_socket"

void signalHandler( int signal )
{
    serverQuit( );
}

void client( int argc, char** argv, int socketFile, struct sockaddr_un* sock )
{
    printf( "Running kegerator daemon client\n" );
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
    serverMain( );
    printf( "Closing kegerator daemon...\n" );
    
    close( socketFile );
    unlink( DAEMON_SOCKET_FILE );
    dataStoreQuit( );
    flowSensorQuit( );
    
    return 0;
}
