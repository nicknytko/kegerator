#ifdef TARGET_RPI

#include <pigpio.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "flowsensor.h"

static unsigned int pulses = 0;
static unsigned int localPulses = 0;
static pthread_mutex_t *pulseMutex = NULL;
static uint32_t period = 0;
static uint32_t lastTick = 0;

#define FLOW_SENSOR_PIN 27
#define UINT32_HALF 0x7FFFFFFF
#define TIMER_RESET_FLOW_PERIOD 0
#define DELAY_RESET_FLOW_PERIOD_MS 50
#define FREQUENCY_CONSTANT 73
#define US_TO_SECONDS 1000000
#define MAX_PULSE_TICK 100000

static void gpioResetFlowPeriod() {
    pthread_mutex_lock(pulseMutex);
    period = 0;
    pthread_mutex_unlock(pulseMutex);

    gpioSetTimerFunc(TIMER_RESET_FLOW_PERIOD, DELAY_RESET_FLOW_PERIOD_MS, NULL);
}

static int gpioChangeState(int gpio, int level, uint32_t tick) {
    if (gpio == FLOW_SENSOR_PIN && level == 1) {
        if (pthread_mutex_trylock(pulseMutex) == 0) {
            
            /* Save the pulse period, to determine the frequency.
               Check for unsigned integer roll over */
            
            if (lastTick > 0x7FFFFFFF && tick < 0x7FFFFFFF) {
                period = tick + (0xFFFFFFFF - lastTick);
            } else {
                period = tick - lastTick;
            }

            /* Try to minimize any stray ticks that may occur.  This
               will happen at the expense of losing one tick every
               pour */
            
            if (period <= MAX_PULSE_TICK) {
                
                /* Add to the pulses counter, and add any local
                   pulses that may have occured */

                pulses++;
                if (localPulses != 0) {
                    pulses += localPulses;
                    localPulses = 0;
                }
            }

            /* Set up a timer to reset the period after a while */
            
            gpioSetTimerFunc(TIMER_RESET_FLOW_PERIOD, DELAY_RESET_FLOW_PERIOD_MS,
                             gpioResetFlowPeriod);
            pthread_mutex_unlock(pulseMutex);
        } else {
            
            /* Couldn't obtain the lock, save the pulse to
               the local pulses counter */

            localPulses++;
        }
        lastTick = tick;
    }
}

unsigned int flowSensorGetPulses() {
    unsigned int retPulses = 0;

    pthread_mutex_lock(pulseMutex);
    retPulses = pulses;
    pthread_mutex_unlock(pulseMutex);

    return retPulses;
}

double flowSensorGetFrequency() {
    uint32_t retPeriod = 0;

    pthread_mutex_lock(pulseMutex);
    retPeriod = period;
    pthread_mutex_unlock(pulseMutex);

    if (retPeriod == 0) {
        return 0;
    } else {
        double freq = 1.0 / ((double)retPeriod / US_TO_SECONDS);
        return freq;
    }
}

double flowSensorGetRate() {
    return flowSensorGetFrequency() / FREQUENCY_CONSTANT;
}

void flowSensorResetPulses() {
    pthread_mutex_lock(pulseMutex);
    pulses = 0;
    pthread_mutex_unlock(pulseMutex);
}

int flowSensorInit() {
    if (gpioInitialise() == PI_INIT_FAILED) {
        printf("Failed to initialise GPIO library\n");
        return 1;
    }
    if (gpioSetMode(FLOW_SENSOR_PIN, PI_INPUT) != 0) {
        printf("Failed to set pin to input mode\n");
        return 2;
    }
    if (gpioSetPullUpDown(FLOW_SENSOR_PIN, PI_PUD_UP) != 0) {
        printf("Failed to set pull up mode\n");
        return 3;
    }
    if (gpioSetAlertFunc(FLOW_SENSOR_PIN, (gpioAlertFunc_t)gpioChangeState) !=
        0) {
        printf("Failed to set state change callback\n");
        return 4;
    }

    pulseMutex = malloc(sizeof(pthread_mutex_t));
    if (pulseMutex == NULL || pthread_mutex_init(pulseMutex, NULL) != 0) {
        printf("Failed to allocate mutex\n");
        return 5;
    }

    return 0;
}

void flowSensorQuit() {
    if (pulseMutex != NULL) {
        free(pulseMutex);
    }

    gpioSetAlertFunc(FLOW_SENSOR_PIN, NULL);
    gpioSetTimerFunc(TIMER_RESET_FLOW_PERIOD, DELAY_RESET_FLOW_PERIOD_MS, NULL);
}

#endif
