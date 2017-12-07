#include <stdio.h>
#include <time.h>

#include "flowsensor.h"
#include "mongoose.h"

static const char* httpPort = "80";
static struct mg_serve_http_opts httpServerOptions;

void broadcast( struct mg_connection* sender, const char* buffer, unsigned int buflen )
{
    struct mg_connection* current;
    for ( current = mg_next( sender->mgr, NULL ); current != NULL;
          current = mg_next( sender->mgr, current ) )
    {
        if ( current != sender && ( current->flags & 0x100 ) != 0 )
        {
            mg_send_websocket_frame( current, WEBSOCKET_OP_TEXT,
                                     buffer, buflen );
        }
    }
}

void eventHandler( struct mg_connection* connection, int event, void* eventData )
{
    switch ( event )
    {
    case MG_EV_WEBSOCKET_FRAME:
    {
        struct websocket_message* wm = (struct websocket_message*) eventData;
        if ( strncmp( wm->data, "zero", wm->size ) == 0 )
        {
            flowSensorResetPulses( );
        }
        break;
    }
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
    /* Fork self, detach and run program as a daemon */
    
    pid_t process_id = fork( );
    if ( process_id > 0 )
    {
        exit( 0 );
    }

    if ( flowSensorInit( ) != 0 )
    {
        return 1;
    }

    /* Initialise mongoose server junk */
    
    struct mg_mgr manager;
    struct mg_connection* connection;
    mg_mgr_init( &manager, NULL );
    connection = mg_bind( &manager, httpPort, eventHandler );
    mg_set_protocol_http_websocket( connection );
    httpServerOptions.document_root = "http";
    httpServerOptions.enable_directory_listing = "false";

    const long BROADCAST_DELAY = 25;
    char broadcastData[64];
    long lastBroadcast = clock( ) - BROADCAST_DELAY;

    const int FLOW_RATE_AVG_AMT = 50;
    double avgFlowRate = 0;
    
    while ( 1 )
    {
        mg_mgr_poll( &manager, 200 );
        double instFlowRate = flowSensorGetRate( );
        avgFlowRate += ( instFlowRate - avgFlowRate )
            / FLOW_RATE_AVG_AMT;

        if ( ( clock( ) - lastBroadcast ) > BROADCAST_DELAY )
        {
            unsigned int pulses = flowSensorGetFrequency( );
            
            snprintf( broadcastData, 64, "pulses: %i", pulses );
            broadcastString.len = strlen( broadcastData );
            broadcast( connection, broadcastString );

            snprintf( broadcastData, 64, "flow: %d", avgFlowRate );
            broadcastString.len = strlen( broadcastData );
            broadcast( connection, broadcastString );
            
            lastBroadcast = clock( );
        }
    }
    mg_mgr_free( &manager );
    
    return 0;
}
