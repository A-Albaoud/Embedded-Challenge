#ifndef SENSOR_H
#define SENSOR_H

#include <Adafruit_ADXL345_U.h>

void sensor_init();
void sensor_read(float &x, float &y, float &z);

#endif
