#ifndef DV8_MQTT_H
#define DV8_MQTT_H

extern _Atomic(float) linear_x;
extern _Atomic(float) angular_z;
extern _Atomic(float) battery_percentage;
extern _Atomic(int) brush_speed;
extern _Atomic(int) battery_is_charging;
extern _Atomic(int) e_stop;
extern _Atomic(int) handbrake;
extern _Atomic(int) direct_status;
extern _Atomic(int) robot_mode;
extern _Atomic(int) safety_mode;

extern void mqtt_module_start(void);

#endif
