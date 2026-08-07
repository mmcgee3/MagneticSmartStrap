#define _BUZZER_CONTROL_H_

#define BUZZER_FREQ_HZ     6000                     // rated resonant frequency, change for other buzzers
#define BUZZER_MODE        LEDC_LOW_SPEED_MODE      // only mode on C3/S3/C6/H2
#define BUZZER_TIMER       LEDC_TIMER_0
#define BUZZER_CHANNEL     LEDC_CHANNEL_0
#define BUZZER_RES         LEDC_TIMER_10_BIT        // 1024 steps
#define BUZZER_DUTY_ON     (1 << (BUZZER_RES - 1))  // 50% is loudest square wave, change for other volumes
#define BUZZER_DUTY_OFF    0

void buzzer_init(void);
void buzzer_set_state(bool state);