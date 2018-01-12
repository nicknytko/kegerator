#ifdef TARGET_DESKTOP

#include "flowsensor.h"

static unsigned int pulses = 0;

int flowSensorInit( )
{
    return 0;
}

void flowSensorQuit( )
{
}

unsigned int flowSensorGetPulses( )
{
    return pulses;
}

double flowSensorGetFrequency( )
{
    return 0;
}

double flowSensorGetRate( )
{
    return 0;
}

void flowSensorResetPulses( )
{
    pulses = 0;
}

#endif
