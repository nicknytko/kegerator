#ifndef FLOW_SENSOR_H_
#define FLOW_SENSOR_H_

/**
 * Initializes the flow sensor to work on the default pin.
 * @return 0 if successful
 */
int flowSensorInit();

/**
 * Cleans up and frees any memory associated with the flow
 * sensor.
 */
void flowSensorQuit();

/**
 * Get the total number of pulses that have occured
 */
unsigned int flowSensorGetPulses();

/**
 * Get the instantaneous frequency of pulses.
 * @return Pulse frequency in Hz
 */
double flowSensorGetFrequency();

/**
 * Gets the instantaneous flow rate of the flow sensor.
 * @return Rate of flow in L/min
 */
double flowSensorGetRate();

/**
 * Resets the value of the pulses to 0
 */
void flowSensorResetPulses();

#endif
