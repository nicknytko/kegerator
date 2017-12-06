#ifndef FLOW_SENSOR_H_
#define FLOW_SENSOR_H_

#define FLOW_SENSOR_PIN 27

int flowSensorInit( );
unsigned int flowSensorGetPulses( );
void flowSensorResetPulses( );

#endif
