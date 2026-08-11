#define _HALL_SENSOR_CONTROLLER_H_

#define MAGNET_PITCH_MM     10.0f
#define COUNTS_PER_MAGNET   4
#define HALL_COUNT_LIMIT    30000
#define HALL_GLITCH_NS      1000

void  hall_sensor_init(void);
int   hall_sensor_get_counts(void);
float hall_sensor_get_distance_mm(void);
void  hall_sensor_reset(void);