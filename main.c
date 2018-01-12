#include <stdio.h>
#include <time.h>

#include "flowsensor.h"
#include "datastore.h"
#include "mongoose.h"

static const char* httpPort = "80";
static struct mg_serve_http_opts httpServerOptions;
static int serverRunning = true;

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

void handleRestApi( struct mg_connection* connection, struct http_message* http )
{
    mg_printf( connection, "HTTP/1.1 200 OK\r\nTransfer-Encoding:"
               " chunked\r\nContent-Type: application/json\r\n\r\n" );
    mg_printf_http_chunk( connection, "{\n" );
    struct list_t* brews = dataStoreGetBrews( );
    uint8_t first = true;
    while ( brews != NULL )
    {
        struct brewdata_t* data = brews->data;
        if ( !first )
        {
            mg_printf_http_chunk( connection, ",\n" );
        }
        first = false;
        
        mg_printf_http_chunk( connection, "\"%s\" : {\n", data->name );
        mg_printf_http_chunk( connection, "\"abv\": %u.%u,\n", data->abv / 10, data->abv % 10 );
        mg_printf_http_chunk( connection, "\"remaining_ml\": %u \n}", data->mLRemaining );
        brews = brews->next;
    }
    mg_printf_http_chunk( connection, "}" );
    mg_send_http_chunk( connection, "", 0 );
}

void eventHandler( struct mg_connection* connection, int event, void* eventData )
{
    switch ( event )
    {
    case MG_EV_WEBSOCKET_FRAME:
    {
        struct websocket_message* wm = (struct websocket_message*) eventData;
        if ( strncmp( (const char*) wm->data, "zero", wm->size ) == 0 )
        {
            flowSensorResetPulses( );
        }
        break;
    }
    case MG_EV_HTTP_REQUEST:
    {
        struct http_message* http = (struct http_message*) eventData;
        if ( mg_vcmp( &( http->uri ), "/api/" ) == 0 )
        {
            handleRestApi( connection, http );
        }
        else
        {
            mg_serve_http( connection, eventData, httpServerOptions );
        }
        break;
    }
    default:
        break;
    }
}

void signalHandler( int signal )
{
    serverRunning = false;
}

int main( )
{
    /* Fork self, detach and run program as a daemon */
    
    pid_t process_id = fork( );
    if ( process_id > 0 )
    {
        printf( "Starting kegerator daemon ...\n" );
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
    
    while ( serverRunning )
    {
        mg_mgr_poll( &manager, 200 );
        double instFlowRate = flowSensorGetFrequency( );
        avgFlowRate += ( instFlowRate - avgFlowRate )
            / FLOW_RATE_AVG_AMT;

        if ( ( clock( ) - lastBroadcast ) > BROADCAST_DELAY )
        {
            unsigned int pulses = flowSensorGetPulses( );
            
            snprintf( broadcastData, 64, "pulses: %i", pulses );
            broadcast( connection, broadcastData, strlen( broadcastData ) );
            snprintf( broadcastData, 64, "flow: %f", avgFlowRate );
            broadcast( connection, broadcastData, strlen( broadcastData ) );
            
            lastBroadcast = clock( );
        }
    }

    printf( "Closing kegerator daemon...\n" );

    mg_mgr_free( &manager );
    dataStoreQuit( );
    flowSensorQuit( );
    
    return 0;
}
