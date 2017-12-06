#include <stdio.h>
#include "flowsensor.h"
#include "mongoose.h"

int main( )
{
    if ( flowSensorInit( ) != 0 )
    {
        return 1;
    }
    
    return 0;
}
