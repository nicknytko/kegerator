#include <pigpio.h>
#include <stdio.h>
#include <threads.h>

#include "flowsensor.h"

static unsigned int pulses = 0;
static unsigned int localPulses = 0;
static mtx_t* pulseMutex;

int gpioChangeState( int gpio, int level, uint32_t tick )
{
    if ( gpio == FLOW_SENSOR_PIN && level == 1 )
    {
        if ( mtx_trylock( pulseMutex ) == thrd_success )
        {
            pulses++;

            if ( localPulses != 0 )
            {
                pulses += localPulses;
                localPulses = 0;
            }
            
            mtx_unlock( pulseMutex );
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

    mtx_lock( pulseMutex );
    retPulses = pulses;
    mtx_unlock( pulseMutex );

    return retPulses;
}

void flowSensorResetPulses( )
{
    mtx_lock( pulseMutex );
    pulses = 0;
    mtx_unlock( pulseMutex );
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

    if ( gpioSetPullUpDown( FLOW_SENSOR_PIN, PI_PUD_DOWN ) != 0 )
    {
        printf( "Failed to set pull down mode\n" );
        return 3;
    }

    if ( gpioSetAlertFunc( FLOW_SENSOR_PIN, (gpioAlertFunc_t) gpioChangeState ) != 0 )
    {
        printf( "Failed to set state change callback\n" );
        return 4;
    }

    pulseMutex = malloc( sizeof( mtx_t ) );
    if ( pulseMutex == NULL ||
         mtx_init( pulseMutex, mtx_plain ) != thrd_success )
    {
        printf( "Failed to allocate mutex\n" );
        return 5;
    }

    return 0;
}
