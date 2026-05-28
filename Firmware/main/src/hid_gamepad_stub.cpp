#include "hid_gamepad.h"

// BT HID not supported on this target (BLE-only, no Classic BT)
void hid_gamepad_init(void *) {}
void hid_gamepad_set_scanning(bool) {}
