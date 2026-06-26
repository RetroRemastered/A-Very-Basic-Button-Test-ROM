#include <stdint.h>
#include "sp_buttons_bg.h"
#include "sp_buttons_pressed_bg.h"
#include "rr_logo_splash.h"

#define REG_DISPCNT (*(volatile uint16_t *)0x04000000)
#define REG_VCOUNT  (*(volatile uint16_t *)0x04000006)
#define REG_KEYINPUT (*(volatile uint16_t *)0x04000130)
#define REG_SOUND1CNT_L (*(volatile uint16_t *)0x04000060)
#define REG_SOUND1CNT_H (*(volatile uint16_t *)0x04000062)
#define REG_SOUND1CNT_X (*(volatile uint16_t *)0x04000064)
#define REG_SOUNDCNT_L (*(volatile uint16_t *)0x04000080)
#define REG_SOUNDCNT_H (*(volatile uint16_t *)0x04000082)
#define REG_SOUNDCNT_X (*(volatile uint16_t *)0x04000084)

#define MODE3 0x0003
#define BG2_ENABLE 0x0400

#define SCREEN_W 240
#define SCREEN_H 160

#define KEY_A      0x0001
#define KEY_B      0x0002
#define KEY_SELECT 0x0004
#define KEY_START  0x0008
#define KEY_RIGHT  0x0010
#define KEY_LEFT   0x0020
#define KEY_UP     0x0040
#define KEY_DOWN   0x0080
#define KEY_R      0x0100
#define KEY_L      0x0200

typedef struct {
    uint16_t key;
    uint16_t pitch;
} SoundButton;

typedef struct {
    uint16_t key;
    int x;
    int y;
    int w;
    int h;
    int slice;
} ButtonRegion;

static volatile uint16_t *const vram = (volatile uint16_t *)0x06000000;

static const SoundButton sound_buttons[] = {
    {KEY_UP, 1650},
    {KEY_LEFT, 1547},
    {KEY_RIGHT, 1673},
    {KEY_DOWN, 1602},
    {KEY_L, 0},
    {KEY_R, 2041},
    {KEY_SELECT, 1714},
    {KEY_START, 1750},
    {KEY_B, 1783},
    {KEY_A, 1797},
};

static const ButtonRegion button_regions[] = {
    {KEY_L, 42, 4, 31, 24, 0},
    {KEY_R, 168, 4, 31, 24, 0},
    {KEY_UP, 52, 41, 56, 56, 1},
    {KEY_RIGHT, 52, 41, 56, 56, 2},
    {KEY_DOWN, 52, 41, 56, 56, 3},
    {KEY_LEFT, 52, 41, 56, 56, 4},
    {KEY_A | KEY_B, 128, 47, 61, 43, 5},
    {KEY_SELECT, 96, 127, 22, 22, 0},
    {KEY_START, 122, 127, 22, 22, 0},
};

static uint16_t current_sound_key = 0;

static void vsync(void) {
    while (REG_VCOUNT >= 160) {}
    while (REG_VCOUNT < 160) {}
}

static void draw_background(void) {
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++) {
        vram[i] = sp_buttons_bg[i];
    }
}

static void draw_splash(void) {
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++) {
        vram[i] = rr_logo_splash[i];
    }
}

static int abs_i(int value) {
    return value < 0 ? -value : value;
}

static int is_inside_dpad_slice(int slice, int px, int py) {
    int dx = px - 80;
    int dy = py - 69;
    int adx = abs_i(dx);
    int ady = abs_i(dy);

    switch (slice) {
    case 1: return dy <= 0 && ady >= adx;
    case 2: return dx >= 0 && adx > ady;
    case 3: return dy > 0 && ady >= adx;
    case 4: return dx < 0 && adx > ady;
    default: return 1;
    }
}

static int is_meaningful_press_pixel(int index) {
    uint16_t base = sp_buttons_bg[index];
    uint16_t pressed = sp_buttons_pressed_bg[index];
    int base_sum = (base & 31) + ((base >> 5) & 31) + ((base >> 10) & 31);
    int pressed_sum = (pressed & 31) + ((pressed >> 5) & 31) + ((pressed >> 10) & 31);

    return base_sum - pressed_sum > 12;
}

static int is_ab_pressed_pixel(uint16_t keys, int px, int py) {
    int bdx = px - 145;
    int bdy = py - 70;
    int adx = px - 172;
    int ady = py - 62;
    int bdist = bdx * bdx + bdy * bdy;
    int adist = adx * adx + ady * ady;
    int bheld = (keys & KEY_B) != 0;
    int aheld = (keys & KEY_A) != 0;

    if (bheld && bdist <= adist) {
        return 1;
    }

    if (aheld && adist < bdist) {
        return 1;
    }

    return 0;
}

static void draw_region(const ButtonRegion *region, uint16_t keys) {
    int held = (keys & region->key) != 0;

    for (int y = 0; y < region->h; y++) {
        int py = region->y + y;
        if ((unsigned)py >= SCREEN_H) {
            continue;
        }

        for (int x = 0; x < region->w; x++) {
            int px = region->x + x;
            if ((unsigned)px >= SCREEN_W) {
                continue;
            }

            int index = py * SCREEN_W + px;
            if (region->slice && !is_inside_dpad_slice(region->slice, px, py)) {
                continue;
            }
            int use_pressed = 0;
            if (region->slice == 5) {
                use_pressed = is_ab_pressed_pixel(keys, px, py);
            } else if (held) {
                use_pressed = 1;
            }
            vram[index] = use_pressed && is_meaningful_press_pixel(index) ? sp_buttons_pressed_bg[index] : sp_buttons_bg[index];
        }
    }
}

static void update_changed_regions(uint16_t previous_keys, uint16_t keys) {
    uint16_t changed = (uint16_t)(previous_keys ^ keys);
    uint16_t dpad = KEY_UP | KEY_LEFT | KEY_RIGHT | KEY_DOWN;

    for (unsigned i = 0; i < sizeof(button_regions) / sizeof(button_regions[0]); i++) {
        if (button_regions[i].slice && (changed & dpad)) {
            draw_region(&button_regions[i], keys);
        } else if (changed & button_regions[i].key) {
            draw_region(&button_regions[i], keys);
        }
    }
}

static void init_sound(void) {
    REG_SOUNDCNT_X = 0x0080;
    REG_SOUNDCNT_L = 0x1177;
    REG_SOUNDCNT_H = 0x0002;
    REG_SOUND1CNT_L = 0x0000;
}

static void play_pitch(uint16_t pitch) {
    REG_SOUND1CNT_L = 0x0000;
    REG_SOUND1CNT_H = (uint16_t)((2 << 6) | (12 << 12));
    REG_SOUND1CNT_X = (uint16_t)(0x8000 | (pitch & 0x07FF));
}

static void stop_sound(void) {
    REG_SOUND1CNT_H = 0x0000;
    REG_SOUND1CNT_X = 0x0000;
}

static uint16_t selected_sound_key(uint16_t keys, uint16_t *pitch) {
    for (unsigned i = 0; i < sizeof(sound_buttons) / sizeof(sound_buttons[0]); i++) {
        if (keys & sound_buttons[i].key) {
            *pitch = sound_buttons[i].pitch;
            return sound_buttons[i].key;
        }
    }

    *pitch = 0;
    return 0;
}

static void ensure_button_sound(uint16_t keys) {
    uint16_t pitch;
    uint16_t sound_key = selected_sound_key(keys, &pitch);

    if (!sound_key) {
        if (current_sound_key || (REG_SOUNDCNT_X & 0x0001)) {
            stop_sound();
        }
        current_sound_key = 0;
        return;
    }

    if (!(REG_SOUNDCNT_X & 0x0080)) {
        init_sound();
    }

    if (sound_key != current_sound_key || !(REG_SOUNDCNT_X & 0x0001)) {
        play_pitch(pitch);
    }
    current_sound_key = sound_key;
}

static uint16_t read_keys(void) {
    return (uint16_t)(~REG_KEYINPUT & 0x03FF);
}

int main(void) {
    init_sound();
    draw_splash();
    REG_DISPCNT = MODE3 | BG2_ENABLE;

    uint16_t keys = 0;
    do {
        vsync();
        keys = read_keys();
    } while (!keys);

    vsync();
    draw_background();

    /* Resample after the slow full-screen copy on real hardware. */
    vsync();
    keys = read_keys();
    update_changed_regions(0, keys);

    /* Trigger after VRAM work, then confirm the held state one frame later. */
    vsync();
    ensure_button_sound(keys);
    vsync();
    uint16_t confirmed_keys = read_keys();
    if (confirmed_keys != keys) {
        update_changed_regions(keys, confirmed_keys);
    }
    ensure_button_sound(confirmed_keys);
    keys = confirmed_keys;

    uint16_t previous_keys = keys;
    for (;;) {
        keys = read_keys();
        if (keys != previous_keys) {
            vsync();
            update_changed_regions(previous_keys, keys);
            previous_keys = keys;
        } else {
            vsync();
        }
        ensure_button_sound(keys);
    }
}
