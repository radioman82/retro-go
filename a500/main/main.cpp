#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_cache.h"
#include "rg_system.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// UAE4all exports
#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "options.h"
#include "memory.h"
#include "custom.h"
#include "disk.h"
#include "xwin.h"
#include "joystick.h"
#include "drawing.h"
#include "keyboard.h"
#include "keybuf.h"
#include "m68k/m68k_intrf.h"

// Retro-Go App & Surfaces
static rg_app_t *app = NULL;

#define BUFFER_COUNT 3
static rg_surface_t *screen_surf[BUFFER_COUNT];
static int current_buf = 0;

extern "C" void flush_audio(void);
extern void check_all_prefs(void);

// Joypad state exported to uae_joy.cpp
extern int rg_joy_dir;
extern int rg_joy_button;
extern int rg_joy0_dir;
extern int rg_joy0_button;

// Mouse coordinates & buttons from custom.cpp
extern int lastmx, lastmy;
extern int buttonstate[3];

// Global inputs and settings
static int mouse_mode = 0; // 0 = Joystick, 1 = Mouse

// Sound submission callback called by sound_retro.cpp
extern "C" void retro_audiocb(signed short int *sound_buffer, int sndbufsize) {
    if (sound_buffer && sndbufsize > 0) {
        rg_audio_submit((const rg_audio_frame_t *)sound_buffer, sndbufsize);
    }
}

// UAE exports from uae_main.cpp
extern void real_main (int argc, char **argv);
extern char uae4all_image_file[128];
extern char *gfx_mem;
extern unsigned gfx_rowbytes;
extern int quit_program;

// Tiny 3x5 font for on-screen FPS and latency display
static void draw_overlay_char(uint16_t *pixels, int x, int y, char c, uint16_t color, uint16_t bg) {
    static const uint16_t glyphs[16] = {
        0x7B6F, // 0
        0x2492, // 1
        0x73E7, // 2
        0x73CF, // 3
        0x5BC9, // 4
        0x79CF, // 5
        0x79EF, // 6
        0x7249, // 7
        0x7BEF, // 8
        0x7BCF, // 9
        0x79E4, // F
        0x7BC4, // P
        0x79CF, // S
        0x5555, // M
        0x0410, // :
        0x0000, // space
    };
    int idx = 15;
    if (c >= '0' && c <= '9') idx = c - '0';
    else if (c == 'f' || c == 'F') idx = 10;
    else if (c == 'p' || c == 'P') idx = 11;
    else if (c == 's' || c == 'S') idx = 12;
    else if (c == 'm' || c == 'M') idx = 13;
    else if (c == ':') idx = 14;

    uint16_t g = glyphs[idx];
    for (int r = 0; r < 5; r++) {
        for (int col = 0; col < 3; col++) {
            bool on = (g >> (14 - (r * 3 + col))) & 1;
            int px = x + col;
            int py = y + r;
            if (px >= 0 && px < 320 && py >= 0 && py < 240) {
                pixels[py * 320 + px] = on ? color : bg;
            }
        }
    }
}

static int g_fps = 0;
static int g_frame_time_ms = 0;

static void draw_fps_overlay(uint16_t *pixels, int fps, int ms) {
    if (fps <= 0) return;
    char buf[32];
    snprintf(buf, sizeof(buf), "%dFPS %dMS", fps, ms);
    int x = 4;
    int y = 4;
    for (int i = 0; buf[i]; i++) {
        draw_overlay_char(pixels, x, y, buf[i], 0x07E0 /* bright green */, 0x0000 /* black */);
        x += 4;
    }
}

// Display update called by UAE's flush_screen()
extern "C" void uae_rg_display_flush(void) {
    if (screen_surf[current_buf] && gfx_mem) {
        // Non-blocking sync with triple buffering:
        // Core 1 display task transmits in the background over SPI DMA.
        // We only yield/wait if the queue is full.
        rg_display_sync(false);
        memcpy(screen_surf[current_buf]->data, gfx_mem, 320 * 240 * 2);
        draw_fps_overlay((uint16_t *)screen_surf[current_buf]->data, g_fps, g_frame_time_ms);
        rg_display_submit(screen_surf[current_buf], RG_DISPLAY_WRITE_NOSYNC);
        current_buf = (current_buf + 1) % BUFFER_COUNT;
    }
}

static void amigainput_poll(void) {
    uint32_t joy = rg_input_read_gamepad();

    static uint32_t prev_joy = 0;
    static bool menu_cancelled = false;
    uint32_t pressed = joy & ~prev_joy;
    uint32_t released = ~joy & prev_joy;

    // Handle Retro-Go Menu (Pin 18) and Option (Pin 8)
    if (joy & RG_KEY_MENU) {
        if (joy & ~RG_KEY_MENU) {
            menu_cancelled = true;
        }
    } else {
        if (prev_joy & RG_KEY_MENU) {
            if (!menu_cancelled) {
                rg_gui_game_menu();
            }
            menu_cancelled = false;
        }
    }

    if (pressed & RG_KEY_OPTION) {
        rg_gui_options_menu();
    }

    // Select toggles Mouse vs Joystick mode
    if (pressed & RG_KEY_SELECT) {
        mouse_mode = !mouse_mode;
        rg_gui_draw_hourglass(); // brief feedback
    }

    // Amiga keyboard shortcuts (essential for menus, cracktros, and in-game pause)
    // X -> SPACE
    if (pressed & RG_KEY_X)  record_key((AK_SPC << 1) | 0);
    if (released & RG_KEY_X) record_key((AK_SPC << 1) | 1);

    // Y -> RETURN
    if (pressed & RG_KEY_Y)  record_key((AK_RET << 1) | 0);
    if (released & RG_KEY_Y) record_key((AK_RET << 1) | 1);

    // START -> ESC (or pause)
    if (pressed & RG_KEY_START)  record_key((AK_ESC << 1) | 0);
    if (released & RG_KEY_START) record_key((AK_ESC << 1) | 1);

    if (!mouse_mode) {
        // Joystick Mode: Standard Port 2 (joy1 in UAE4All)
        int left  = (joy & RG_KEY_LEFT)  ? 1 : 0;
        int right = (joy & RG_KEY_RIGHT) ? 1 : 0;
        int top   = (joy & RG_KEY_UP)    ? 1 : 0;
        int bot   = (joy & RG_KEY_DOWN)  ? 1 : 0;

        // Amiga JOY1DAT Gray-code / quadrature encoding
        if (left)  top = !top;
        if (right) bot = !bot;
        rg_joy_dir = bot | (right << 1) | (top << 8) | (left << 9);

        // Fire 1 (Button 1) on Port 2: Active on B or A
        int btn = 0;
        if (joy & (RG_KEY_A | RG_KEY_B)) btn |= 1;
        rg_joy_button = btn;

        // Mouse buttons on Port 1:
        // Left mouse click on A or B so cracktros / menus that require mouse click advance!
        buttonstate[0] = (joy & (RG_KEY_A | RG_KEY_B)) ? 1 : 0;
        buttonstate[2] = 0;
        rg_joy0_dir = 0;
        rg_joy0_button = buttonstate[0];
    } else {
        // Mouse Mode: D-Pad moves cursor, B is Left Click, A is Right Click
        rg_joy_dir = 0;
        rg_joy_button = 0;

        const int MOUSE_SPEED = 4;
        if (joy & RG_KEY_LEFT)  lastmx -= MOUSE_SPEED;
        if (joy & RG_KEY_RIGHT) lastmx += MOUSE_SPEED;
        if (joy & RG_KEY_UP)    lastmy -= MOUSE_SPEED;
        if (joy & RG_KEY_DOWN)  lastmy += MOUSE_SPEED;

        if (lastmx < 0) lastmx = 0;
        if (lastmx >= 320) lastmx = 319;
        if (lastmy < 0) lastmy = 0;
        if (lastmy >= 240) lastmy = 239;

        buttonstate[0] = (joy & RG_KEY_B) ? 1 : 0; // Left Mouse Button on B
        buttonstate[2] = (joy & RG_KEY_A) ? 1 : 0; // Right Mouse Button on A
        rg_joy0_button = buttonstate[0];
    }

    prev_joy = joy;
}

static bool load_state_handler(const char *path) {
    return false;
}

static bool save_state_handler(const char *path) {
    return false;
}

static bool reset_handler(bool hard) {
    m68k_reset();
    return true;
}

static char g_detected_bios[256] = { 0 };
static char g_detected_adf[256] = { 0 };

static void boot_checkpoint(const char *msg) {
    RG_LOGI("BOOT: %s", msg);
    FILE *f = fopen("/sd/retro-go/a500_boot.log", "a");
    if (f) {
        fprintf(f, "[A500] %s\n", msg);
        fflush(f);
        fclose(f);
    }
}

extern char changed_df[2][128];
extern int m68k_speed;
extern void check_prefs_changed_cpu(void);

// Called by uae_main.cpp immediately after default_prefs()
extern "C" void update_prefs_retrocfg(void) {
    boot_checkpoint("update_prefs_retrocfg called");
    prefs_chipmem_size = 0x00080000; // 512 KB standard OCS Chip RAM (Kickstart 1.3 requirement)
    prefs_gfx_framerate = 0; // 0 = Standard rendering (maximum game compatibility)
    changed_gfx_framerate = 0;
    check_all_prefs();
    m68k_speed = 1; // Standard scanline timing (preserves VBLANK and bottom raster for 1943 etc.)
    check_prefs_changed_cpu();

    if (g_detected_bios[0]) {
        strncpy(romfile, g_detected_bios, sizeof(romfile) - 1);
        romfile[sizeof(romfile) - 1] = 0;
    } else {
        romfile[0] = 0;
    }

    if (g_detected_adf[0]) {
        strncpy(prefs_df[0], g_detected_adf, sizeof(prefs_df[0]) - 1);
        prefs_df[0][sizeof(prefs_df[0]) - 1] = 0;
        strncpy(changed_df[0], g_detected_adf, sizeof(changed_df[0]) - 1);
        changed_df[0][sizeof(changed_df[0]) - 1] = 0;
        strncpy(uae4all_image_file, g_detected_adf, sizeof(uae4all_image_file) - 1);
        uae4all_image_file[sizeof(uae4all_image_file) - 1] = 0;
        real_changed_df[0] = 0;
    } else {
        prefs_df[0][0] = 0;
        changed_df[0][0] = 0;
        uae4all_image_file[0] = 0;
        real_changed_df[0] = 0;
    }

    prefs_df[1][0] = 0;
    changed_df[1][0] = 0;
    real_changed_df[1] = 0;
    produce_sound = 2; // Enable sound generation (Paula standard mode)
    changed_produce_sound = 2;
}

static void run_amiga_benchmark(void) {
    boot_checkpoint("Running synthetic Amiga benchmark...");

    // 1. CPU Arithmetic Benchmark: 2,000,000 operations
    int64_t t0 = rg_system_timer();
    volatile uint32_t a = 0x12345678, b = 0x87654321, c = 0;
    for (int i = 0; i < 2000000; i++) {
        a = (a ^ (b + i)) + (c >> 1);
        b = (b - a) ^ (i << 2);
        c += (a * 3) ^ b;
    }
    int64_t t1 = rg_system_timer();
    double cpu_time_sec = (t1 - t0) / 1000000.0;
    double mips = (2.0 / cpu_time_sec);
    double a500_pct = (mips / 1.13) * 100.0;

    // 2. PSRAM Bandwidth Benchmark: Copy 320x240 RGB565 50 times
    uint8_t *src_buf = (uint8_t *)screen_surf[0]->data;
    uint8_t *dst_buf = (uint8_t *)screen_surf[1]->data;
    t0 = rg_system_timer();
    for (int i = 0; i < 50; i++) {
        memcpy(dst_buf, src_buf, 320 * 240 * 2);
    }
    t1 = rg_system_timer();
    double copy_time_sec = (t1 - t0) / 1000000.0;
    double mb_per_sec = (50.0 * 320.0 * 240.0 * 2.0 / (1024.0 * 1024.0)) / copy_time_sec;

    // 3. Display submission Benchmark: Submit 30 frames
    t0 = rg_system_timer();
    for (int i = 0; i < 30; i++) {
        rg_display_submit(screen_surf[0], RG_DISPLAY_WRITE_NOSYNC);
        rg_display_sync(true);
    }
    t1 = rg_system_timer();
    double disp_time_sec = (t1 - t0) / 1000000.0;
    double disp_fps = 30.0 / disp_time_sec;
    double disp_ms = (disp_time_sec / 30.0) * 1000.0;

    char report[384];
    snprintf(report, sizeof(report),
        "A500 Benchmark Results:\n\n"
        "CPU Core:     %.1f MIPS (%.0f%% A500)\n"
        "PSRAM Copy:   %.1f MB/s\n"
        "Display DMA:  %.1f FPS (%.1f ms)\n\n"
        "Press A to continue.",
        mips, a500_pct, mb_per_sec, disp_fps, disp_ms);

    boot_checkpoint(report);
    rg_gui_alert("Amiga 500 Performance", report);
}

// Retro-Go Main Entry Point
extern "C" void app_main(void) {
    boot_checkpoint("app_main started");

    const rg_handlers_t handlers = {
        .loadState = &load_state_handler,
        .saveState = &save_state_handler,
        .reset = &reset_handler,
    };

    app = rg_system_reinit(22050, &handlers, NULL);
    rg_system_set_tick_rate(50); // PAL 50 Hz

    boot_checkpoint("rg_system initialized, allocating display surfaces");

    // Allocate 320x240 RGB565 display surfaces in PSRAM (triple buffering)
    for (int i = 0; i < BUFFER_COUNT; i++) {
        screen_surf[i] = rg_surface_create(320, 240, RG_PIXEL_565_LE, MEM_SLOW);
        if (!screen_surf[i]) {
            boot_checkpoint("FATAL: Failed to allocate display surfaces");
            rg_system_panic("a500", "Failed to allocate display surfaces");
            return;
        }
    }
    boot_checkpoint("Display surfaces allocated successfully (triple buffered)");

    // If user holds button X at boot, run benchmark
    uint32_t init_joy = rg_input_read_gamepad();
    if (init_joy & RG_KEY_X) {
        run_amiga_benchmark();
    }

    // Check Kickstart 1.3 BIOS
    const char *bios_paths[] = {
        "/sd/retro-go/bios/kick13.rom",
        "/sd/retro-go/bios/kick.rom",
        "/sd/roms/a500/kick13.rom",
        "/sd/roms/a500/kick.rom",
    };
    bool bios_found = false;
    for (size_t i = 0; i < sizeof(bios_paths)/sizeof(bios_paths[0]); i++) {
        if (rg_storage_exists(bios_paths[i])) {
            strncpy(g_detected_bios, bios_paths[i], sizeof(g_detected_bios) - 1);
            bios_found = true;
            char msg[384];
            snprintf(msg, sizeof(msg), "Found Kickstart ROM: %s", g_detected_bios);
            boot_checkpoint(msg);
            break;
        }
    }
    if (!bios_found) {
        boot_checkpoint("Kickstart ROM not found, will use Ersatz Kickstart");
        g_detected_bios[0] = 0;
    }

    // Setup ADF image path
    if (app->romPath && strlen(app->romPath) > 0) {
        strncpy(g_detected_adf, app->romPath, sizeof(g_detected_adf) - 1);
        char msg[384];
        snprintf(msg, sizeof(msg), "Loading ADF image: %s", g_detected_adf);
        boot_checkpoint(msg);
    } else {
        g_detected_adf[0] = 0;
        boot_checkpoint("No ADF loaded, booting to Kickstart screen");
    }

    // Setup UAE arguments and start
    const char *argv[] = { "uae4all", NULL };
    int argc = 1;

    boot_checkpoint("Calling real_main()...");
    real_main(argc, (char **)argv);
    boot_checkpoint("real_main() completed");

    // Main emulation loop
    boot_checkpoint("Entering UAE4all 68000 emulation loop...");

    quit_program = 2;
    reset_frameskip();

    int frame_counter = 0;
    int fps_frames = 0;
    int64_t fps_time = rg_system_timer();

    while (!rg_system_exit_called()) {
        int64_t startTime = rg_system_timer();

        amigainput_poll();

        // Run 1 frame of m68k emulation
        m68k_go(1);

        // Flush any remaining audio generated during this frame directly to I2S
        flush_audio();

        int64_t frameTime = rg_system_timer() - startTime;

        fps_frames++;
        int64_t now = rg_system_timer();
        if (now - fps_time >= 1000000LL) {
            g_fps = (int)((fps_frames * 1000000LL) / (now - fps_time));
            g_frame_time_ms = (int)(frameTime / 1000);
            fps_frames = 0;
            fps_time = now;
            RG_LOGI("[A500] FPS: %d | Frame: %d ms", g_fps, g_frame_time_ms);
        }

        if (frame_counter < 5) {
            char msg[64];
            snprintf(msg, sizeof(msg), "Frame %d rendered successfully", frame_counter);
            boot_checkpoint(msg);
            frame_counter++;
        }

        rg_system_tick(frameTime);
        rg_system_sync_frame(startTime);
    }

    boot_checkpoint("Emulation exiting...");
    rg_system_exit();
}
