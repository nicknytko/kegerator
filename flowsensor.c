#include <pigpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "flowsensor.h"

static unsigned int pulses = 0;
static unsigned int localPulses = 0;
static pthread_mutex_t* pulseMutex;

int gpioChangeState( int gpio, int level, uint32_t tick )
{
    if ( gpio == FLOW_SENSOR_PIN && level == 1 )
    {
        if ( pthread_mutex_trylock( pulseMutex ) == 0 )
        {
            pulses++;

            if ( localPulses != 0 )
            {
                pulses += localPulses;
                localPulses = 0;
            }
            
            pthread_mutex_unlock( pulseMutex );
        }
        else
        {
            localPulses++;
        }
    }
}

unsigned int flowSensorGetPulses( )
{
    unsigned int retPulses = 0;

    pthread_mutex_lock( pulseMutex );
    retPulses = pulses;
    pthread_mutex_unlock( pulseMutex );

    return retPulses;
}

void flowSensorResetPulses( )
{
    pthread_mutex_lock( pulseMutex );
    pulses = 0;
    pthread_mutex_unlock( pulseMutex );
}

int flowSensorInit( )
{
    if ( gpioInitialise( ) == PI_INIT_FAILED )
    {
        printf( "Failed to initialise GPIO library\n" );
        return 1;
    }

    if ( gpioSetMode( FLOW_SENSOR_PIN, PI_INPUT ) != 0 )
    {
        printf( "Failed to set pin to input mode\n" );
        return 2;
    }

    if ( gpioSetPullUpDown( FLOW_SENSOR_PIN, PI_PUD_UP ) != 0 )
    {
        printf( "Failed to set pull down mode\n" );
        return 3;
    }

    if ( gpioSetAlertFunc( FLOW_SENSOR_PIN, (gpioAlertFunc_t) gpioChangeState ) != 0 )
    {
        printf( "Failed to set state change callback\n" );
        return 4;
    }

    pulseMutex = malloc( sizeof( pthread_mutex_t ) );
    if ( pulseMutex == NULL ||
         pthread_mutex_init( pulseMutex, NULL ) != 0 )
    {
        printf( "Failed to allocate mutex\n" );
        return 5;
    }

    return 0;
}
