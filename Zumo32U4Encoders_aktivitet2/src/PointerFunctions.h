#ifdef Stop
#include <Arduino.h>
#include <Zumo32U4.h>
#include <stdlib.h>

using namespace std;

Zumo32U4Encoders  encoders;
Zumo32U4Motors    motors;
Zumo32U4ButtonA   buttonA;
Zumo32U4LCD       LCD;

#define motor_speed 300

double  metersStamp, 
        metersNow,
        *pRevs;

double * getRevolutionsCount  ()                          ;
double * getMetersCount       ()                          ;

void     driveMotors          (double distance_in_meters) ;
#endif