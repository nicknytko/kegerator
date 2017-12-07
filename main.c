#include <stdio.h>
#include <sys/time.h>

#include "flowsensor.h"
#include "mongoose.h"

static const char* httpPort = "80";
static struct mg_serve_http_opts httpServerOptions;

void broadcast( struct mg_connection* sender, const struct mg_str msg )
{
    struct mg_connection* current;
    char buffer[512];
    char address[32];

    mg_sock_addr_to_str( sender->sa, address, sizeof( address ),
                         MG_SOCK_STRINGIFY_IP, MG_SOCK_STRINGIFY_PORT );

    snprintf( buffer, sizeof( buffer ), addr, (int) msg.len, msg.p );

    for ( current = mg_next( sender->mgr, NULL ); current != NULL;
          current = mg_next( sender->mgr, current ) )
    {
        if ( current != sender )
        {
            mg_send_websocket_frame( current, WEBSOCKET_OP_TEXT, buffer, strlen( buffer ) );
        }
    }
}

void eventHandler( struct mg_connection* connection, int event, void* eventData )
{
    switch ( event )
    {
    case MG_EV_HTTP_REQUEST:
    {
        mg_serve_http( connection, (struct http_message*) eventData, httpServerOptions );
        break;
    }
    default:
        break;
    }
}

int main( )
{
    if ( flowSensorInit( ) != 0 )
    {
        return 1;
    }

    struct mg_mgr manager;
    struct mg_connection* connection;
    mg_mgr_init( &mgr, NULL );
    connection = mg_bind( &mgr, httpPort, eventHandler );
    mg_set_protocol_http_websocket( connection );
    httpServerOptions.document_root = "http";
    httpServerOptions.enable_directory_listing = "false";

    char broadcastData[64];
    struct mg_str broadcastString;
    broadcastString.p = broadcastData;
    
    while ( 1 )
    {
        mg_mgr_poll( &mgr, 200 );
        
        unsigned int pulses = flowSensorGetPulses( );
        snprintf( broadcastData, 64, "pulses: %i", pulses );
        broadcastString.len = strlen( broadcastString );
        
        broadcast( connection, broadcastString );
    }
    mg_mgr_free( &mgr );
    
    return 0;
}
