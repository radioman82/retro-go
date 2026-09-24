#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "options.h"
#include "gui.h"

int show_message = 0;
char *show_message_str = NULL;

char uae4all_image_file[128] = { 0 };
char uae4all_image_file2[128] = { 0 };

static int dummy_tablas_ajuste[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int *tabla_ajuste = dummy_tablas_ajuste;

int gui_init (void) {
    return 0;
}

int gui_update (void) {
    return 0;
}

void gui_exit (void) {}

void gui_led (int led, int on) {
    extern unsigned int gui_ledstate;
    gui_ledstate &= ~(1 << led);
    if (on) gui_ledstate |= (1 << led);
}

void gui_handle_events (void) {}
void gui_filename (int num, const char *name) {}
void gui_fps (int fps) {}
void gui_changesettings (void) {}
void gui_lock (void) {}
void gui_unlock (void) {}
void gui_set_message(char *msg, int t) {}
void gui_show_window_bar(int per, int max, int case_title) {}
void gui_update_gfx (void) {}
void uae4all_update_time(void) {}
void uae4all_show_time(void) {}
