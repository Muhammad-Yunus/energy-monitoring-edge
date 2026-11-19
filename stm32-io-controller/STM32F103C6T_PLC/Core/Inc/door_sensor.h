#ifndef DOOR_SENSOR_H
#define DOOR_SENSOR_H

#include "main.h"

#define DOOR_COOLDOWN_MS  2000

void DoorSensor_Init(void);

void DoorSensor_ExtiHandler(void);

int  DoorSensor_IsChanged(void);
void DoorSensor_ClearFlag(void);

uint8_t DoorSensor_GetState(void);

int  DoorSensor_Post(uint8_t state);

#endif
