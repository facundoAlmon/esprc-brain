// Entry point. BT stack is initialized later in hid_gamepad_init() (called
// from setupGamepad() after NVS and WiFi are ready).

// Declared in sketch.cpp
void app_task_start(void);

int app_main(void)
{
    app_task_start();
    return 0;
}
