#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "options.h"
#include "joystick.h"

int nr_joysticks = 1;

// Global inputs updated by retro-go
int rg_joy_dir = 0;
int rg_joy_button = 0;
int rg_joy0_dir = 0;
int rg_joy0_button = 0;

void init_joystick (void) {
    nr_joysticks = 2;
}

void close_joystick (void) {}

void read_joystick (int nr, unsigned int *dir, int *button) {
    if (nr == 0) {
        // Amiga Joystick Port 2 (Standard game port, custom.cpp getjoystate(0, &joy1dir, &joy1button))
        *dir = rg_joy_dir;
        *button = rg_joy_button;
    } else if (nr == 1) {
        // Amiga Joystick Port 1 (Mouse / 2nd port, custom.cpp getjoystate(1, &joy0dir, &joy0button))
        *dir = rg_joy0_dir;
        *button = rg_joy0_button;
    } else {
        *dir = 0;
        *button = 0;
    }
}
