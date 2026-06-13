/*
 * Moana and the Lemon Apocalypse - C cartridge for PRG32
 *
 * A tiny arcade chase game for the PRG32 runtime.  Moana gathers lemons before
 * they expire, races chickens, throws basket lemons at the rooster, and escapes
 * through a border door when each screen quota is complete.
 */

#include "prg32.h"
#include <stdint.h>

#define SCREEN_W 320
#define SCREEN_H 200
#define SPRITE_SIZE 24
#define PLAYER_W SPRITE_SIZE
#define PLAYER_H SPRITE_SIZE
#define CHICKEN_W SPRITE_SIZE
#define CHICKEN_H SPRITE_SIZE
#define ROOSTER_W SPRITE_SIZE
#define ROOSTER_H SPRITE_SIZE
#define LEMON_W SPRITE_SIZE
#define LEMON_H SPRITE_SIZE
#define MAX_TREES 6
#define MAX_LEMONS 16
#define MAX_CHICKENS 5
#define MAX_SHOTS 3
#define FPS 30
#define SCREEN_TIME_FRAMES (150u * FPS)
#define MAGIC_TIME_FRAMES (30u * FPS)
#define BAD_TIME_FRAMES (15u * FPS)
#define ENERGY_FRAMES (12u * FPS)
#define QUOTA_BASE 18
#define FINAL_LEVEL 7

#define COLOR_SKY       0x7e3f
#define COLOR_SEA       0x04b7
#define COLOR_GRASS     0x4de7
#define COLOR_GRASS_DK  0x1b43
#define COLOR_SAND      0xff18
#define COLOR_TREE      0x4ae0
#define COLOR_TRUNK     0x8a22
#define COLOR_LEMON     0xffa0
#define COLOR_MAGIC     0x07ff
#define COLOR_ENERGY    0xfd20
#define COLOR_BAD       0x7a86
#define COLOR_SKIN      0xff17
#define COLOR_HAIR      0x8208
#define COLOR_EYE       0x05bf
#define COLOR_DRESS     0x03ef
#define COLOR_CHICKEN   0xff7a
#define COLOR_ROOSTER   0xf800
#define COLOR_SHADOW    0x2104
#define COLOR_DOOR      0x5aeb
#define COLOR_DOOR_LIT  0xffe0

#define BTN_THROW PRG32_BTN_A
#define BTN_SELECT PRG32_BTN_SELECT
#define BTN_MOVE_MASK (PRG32_BTN_LEFT | PRG32_BTN_RIGHT | PRG32_BTN_UP | PRG32_BTN_DOWN)

typedef enum {
    STATE_SPLASH = 0,
    STATE_PLAY = 1,
    STATE_ESCAPE = 2,
    STATE_LEVEL_CLEAR = 3,
    STATE_GAME_OVER = 4,
    STATE_FINAL_VICTORY = 5
} GameState;

typedef enum {
    LEMON_NORMAL = 0,
    LEMON_MAGIC = 1,
    LEMON_ENERGY = 2,
    LEMON_BAD = 3
} LemonKind;

typedef enum {
    EFFECT_NONE = 0,
    EFFECT_MAGIC = 1,
    EFFECT_ENERGY = 2,
    EFFECT_BAD = 3,
    EFFECT_HURT = 4
} EffectKind;

typedef struct {
    int16_t x;
    int16_t y;
    int8_t active;
    uint8_t age;
    uint8_t ttl;
    LemonKind kind;
} Lemon;

typedef struct {
    int16_t x;
    int16_t y;
    int8_t dx;
    int8_t dy;
    int8_t active;
} Shot;

typedef struct {
    int16_t x;
    int16_t y;
    int8_t dx;
    int8_t dy;
    int8_t facing;
} Chicken;

typedef struct {
    int16_t x;
    int16_t y;
    int8_t active;
    int8_t scared;
    int8_t facing;
    uint16_t cooldown;
} Rooster;

static const int16_t tree_x[MAX_TREES] = { 58, 154, 248, 104, 204, 286 };
static const int16_t tree_y[MAX_TREES] = { 42, 30, 44, 56, 54, 28 };
static const uint16_t melody[] = {
    392, 0, 494, 0, 440, 523, 0, 392,
    0, 330, 392, 0, 587, 0, 523, 440,
    0, 392, 494, 0, 659, 587, 0, 523,
    0, 440, 392, 0, 330, 0, 392, 0
};

static GameState state;
static Lemon lemons[MAX_LEMONS];
static Shot shots[MAX_SHOTS];
static Chicken chickens[MAX_CHICKENS];
static Rooster rooster;
static uint32_t rng_state;
static uint32_t frame_no;
static uint32_t screen_timer;
static uint16_t score;
static uint16_t basket;
static uint16_t collected_this_screen;
static uint8_t screen_no;
static uint8_t quota;
static uint8_t energy_timer;
static uint8_t hurt_timer;
static uint8_t effect_timer;
static EffectKind effect_kind;
static uint8_t next_screen_after_clear;
static uint8_t last_dir;
static int8_t moana_facing;
static uint32_t prev_input;
static int16_t moana_x;
static int16_t moana_y;
static int16_t door_x;
static int16_t door_y;
static uint8_t door_side;

static int abs_i(int v) { return v < 0 ? -v : v; }

static uint32_t rnd(void) {
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state;
}

static int clamp_i(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int overlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

static uint8_t chicken_count_for_screen(uint8_t s) {
    if (s < 2u) return 0;
    if (s > 6u) return MAX_CHICKENS;
    return (uint8_t)(s - 1u);
}

static uint8_t tree_count_for_screen(uint8_t s) {
    if (s < 1u) return 1;
    if (s > MAX_TREES) return MAX_TREES;
    return s;
}

static uint8_t rooster_enabled_for_screen(uint8_t s) {
    return s >= 7u;
}

static void trigger_effect(EffectKind kind, uint8_t frames) {
    effect_kind = kind;
    effect_timer = frames;
}

static void draw_num2(int x, int y, uint16_t value, uint16_t fg) {
    char s[3];
    value %= 100;
    s[0] = (char)('0' + value / 10);
    s[1] = (char)('0' + value % 10);
    s[2] = 0;
    prg32_gfx_text8(x, y, s, fg, PRG32_COLOR_BLACK);
}

static void draw_num3(int x, int y, uint16_t value, uint16_t fg) {
    char s[4];
    value %= 1000;
    s[0] = (char)('0' + value / 100);
    s[1] = (char)('0' + (value / 10) % 10);
    s[2] = (char)('0' + value % 10);
    s[3] = 0;
    prg32_gfx_text8(x, y, s, fg, PRG32_COLOR_BLACK);
}

static void draw_num5(int x, int y, uint16_t value, uint16_t fg) {
    char s[6];
    s[0] = (char)('0' + value / 10000);
    s[1] = (char)('0' + (value / 1000) % 10);
    s[2] = (char)('0' + (value / 100) % 10);
    s[3] = (char)('0' + (value / 10) % 10);
    s[4] = (char)('0' + value % 10);
    s[5] = 0;
    prg32_gfx_text8(x, y, s, fg, PRG32_COLOR_BLACK);
}

static void play_melody_tick(void) {
    if (state == STATE_FINAL_VICTORY) return;
    if ((frame_no % 8u) == 0u) {
        uint16_t note = melody[(frame_no / 8u) % (sizeof(melody) / sizeof(melody[0]))];
        if (note) prg32_audio_beep(note, 14);
    }
}

static void play_victory_sound_tick(void) {
    static const uint16_t notes[] = { 523, 659, 784, 1047 };
    if ((frame_no % 6u) == 0u) {
        uint8_t step = (uint8_t)((frame_no / 6u) & 3u);
        prg32_audio_beep(notes[step], 35);
    }
}

static void play_final_song_tick(void) {
    static const uint16_t notes[] = {
        523, 659, 784, 1047, 988, 1047, 1175, 1319,
        1047, 784, 880, 988, 1047, 0, 1319, 1568
    };
    if ((frame_no % 7u) == 0u) {
        uint16_t note = notes[(frame_no / 7u) % (sizeof(notes) / sizeof(notes[0]))];
        if (note) prg32_audio_beep(note, 45);
    }
}

static void reset_chickens(void) {
    for (uint8_t i = 0; i < MAX_CHICKENS; ++i) {
        chickens[i].x = (int16_t)(18 + i * 58);
        chickens[i].y = (int16_t)(148 + ((i & 1u) ? 14 : 0));
        chickens[i].dx = (i & 1u) ? 1 : -1;
        chickens[i].dy = (i & 2u) ? 1 : -1;
        chickens[i].facing = chickens[i].dx;
    }
}

static void choose_door(void) {
    door_side = (uint8_t)(rnd() & 3u);
    if (door_side == 0) {
        door_x = 2;
        door_y = (int16_t)(42 + (rnd() % 116u));
    } else if (door_side == 1) {
        door_x = SCREEN_W - SPRITE_SIZE - 2;
        door_y = (int16_t)(42 + (rnd() % 116u));
    } else if (door_side == 2) {
        door_x = (int16_t)(34 + (rnd() % 236u));
        door_y = 18;
    } else {
        door_x = (int16_t)(34 + (rnd() % 236u));
        door_y = SCREEN_H - SPRITE_SIZE - 2;
    }
}

static void start_screen(uint8_t next_screen) {
    for (uint8_t i = 0; i < MAX_LEMONS; ++i) lemons[i].active = 0;
    for (uint8_t i = 0; i < MAX_SHOTS; ++i) shots[i].active = 0;
    reset_chickens();
    rooster.active = 0;
    rooster.scared = 0;
    rooster.cooldown = 150;
    moana_x = 152;
    moana_y = 102;
    last_dir = 1;
    moana_facing = 1;
    screen_no = next_screen;
    quota = (uint8_t)(QUOTA_BASE + (screen_no - 1u) * 4u);
    collected_this_screen = 0;
    screen_timer = SCREEN_TIME_FRAMES;
    energy_timer = 0;
    hurt_timer = 0;
    effect_timer = 0;
    effect_kind = EFFECT_NONE;
    state = STATE_PLAY;
}

static void start_new_game(void) {
    rng_state = 0x4d6f616eu;
    frame_no = 0;
    score = 0;
    basket = 0;
    prev_input = 0;
    start_screen(1);
}

static LemonKind pick_lemon_kind(void) {
    uint32_t r = rnd() % 100u;
    if (r < 6u) return LEMON_MAGIC;
    if (r < 13u) return LEMON_ENERGY;
    if (r < 21u) return LEMON_BAD;
    return LEMON_NORMAL;
}

static void spawn_lemon_from_tree(uint8_t tree) {
    for (uint8_t i = 0; i < MAX_LEMONS; ++i) {
        Lemon *l = &lemons[i];
        if (!l->active) {
            l->x = (int16_t)(tree_x[tree] - 12 + (rnd() % 25u));
            l->y = (int16_t)(tree_y[tree] + 24);
            l->active = 1;
            l->age = 0;
            l->ttl = (uint8_t)(130 + (rnd() % 90u));
            l->kind = pick_lemon_kind();
            return;
        }
    }
}

static void spawn_lemons_from_active_trees(void) {
    uint8_t active_trees = tree_count_for_screen(screen_no);
    for (uint8_t tree = 0; tree < active_trees; ++tree) spawn_lemon_from_tree(tree);
}

static void update_lemons(void) {
    uint8_t spawn_every = (uint8_t)(36 - clamp_i(screen_no * 3, 0, 18));
    if ((frame_no % spawn_every) == 0u) spawn_lemons_from_active_trees();

    for (uint8_t i = 0; i < MAX_LEMONS; ++i) {
        Lemon *l = &lemons[i];
        if (!l->active) continue;
        if (l->y < 170) l->y += 1 + ((l->age & 7u) == 0u);
        if (++l->age > l->ttl) l->active = 0;
    }
}

static void collect_lemon(Lemon *l) {
    l->active = 0;
    if (l->kind == LEMON_BAD) {
        screen_timer = (screen_timer > BAD_TIME_FRAMES) ? screen_timer - BAD_TIME_FRAMES : 1;
        trigger_effect(EFFECT_BAD, 36);
        prg32_audio_beep(110, 120);
        return;
    }
    if (l->kind == LEMON_MAGIC) {
        screen_timer += MAGIC_TIME_FRAMES;
        score += 75;
        trigger_effect(EFFECT_MAGIC, 42);
        prg32_audio_beep(880, 90);
    } else if (l->kind == LEMON_ENERGY) {
        energy_timer = 255;
        score += 50;
        trigger_effect(EFFECT_ENERGY, 55);
        prg32_audio_beep(740, 70);
    } else {
        score += 20;
        prg32_audio_beep(660, 35);
    }
    basket++;
    collected_this_screen++;
    if (collected_this_screen >= quota && state == STATE_PLAY) {
        state = STATE_ESCAPE;
        choose_door();
        prg32_audio_beep(988, 140);
    }
}

static void update_moana(uint32_t input) {
    int speed = energy_timer ? 4 : 3;
    if (energy_timer) energy_timer--;
    if (hurt_timer) hurt_timer--;
    if (effect_timer) {
        effect_timer--;
        if (!effect_timer) effect_kind = EFFECT_NONE;
    }
    if (input & PRG32_BTN_LEFT) {
        moana_x -= speed;
        last_dir = 3;
        moana_facing = -1;
    }
    if (input & PRG32_BTN_RIGHT) {
        moana_x += speed;
        last_dir = 1;
        moana_facing = 1;
    }
    if (input & PRG32_BTN_UP) {
        moana_y -= speed;
        last_dir = 0;
    }
    if (input & PRG32_BTN_DOWN) {
        moana_y += speed;
        last_dir = 2;
    }
    moana_x = (int16_t)clamp_i(moana_x, 4, SCREEN_W - PLAYER_W - 4);
    moana_y = (int16_t)clamp_i(moana_y, 22, SCREEN_H - PLAYER_H - 4);

    for (uint8_t i = 0; i < MAX_LEMONS; ++i) {
        Lemon *l = &lemons[i];
        if (l->active && overlap(moana_x, moana_y, PLAYER_W, PLAYER_H, l->x, l->y, LEMON_W, LEMON_H)) {
            collect_lemon(l);
        }
    }
}

static void throw_lemon(void) {
    if (basket == 0) return;
    for (uint8_t i = 0; i < MAX_SHOTS; ++i) {
        Shot *s = &shots[i];
        if (!s->active) {
            static const int8_t vx[4] = { 0, 5, 0, -5 };
            static const int8_t vy[4] = { -5, 0, 5, 0 };
            s->x = moana_x + 6;
            s->y = moana_y + 6;
            s->dx = vx[last_dir];
            s->dy = vy[last_dir];
            s->active = 1;
            basket--;
            prg32_audio_beep(520, 30);
            return;
        }
    }
}

static void update_shots(void) {
    for (uint8_t i = 0; i < MAX_SHOTS; ++i) {
        Shot *s = &shots[i];
        if (!s->active) continue;
        s->x += s->dx;
        s->y += s->dy;
        if (s->x < -8 || s->x > SCREEN_W || s->y < 10 || s->y > SCREEN_H) s->active = 0;
        if (s->active && rooster.active && overlap(s->x, s->y, 6, 6, rooster.x, rooster.y, ROOSTER_W, ROOSTER_H)) {
            s->active = 0;
            rooster.scared = 1;
            rooster.cooldown = 210;
            score += 100;
            trigger_effect(EFFECT_MAGIC, 36);
            prg32_audio_beep(1047, 120);
        }
    }
}

static void update_chickens(void) {
    uint8_t active_chickens = chicken_count_for_screen(screen_no);
    for (uint8_t i = 0; i < active_chickens; ++i) {
        Chicken *c = &chickens[i];
        int target_x = moana_x;
        int target_y = moana_y;
        if (state == STATE_PLAY) {
            for (uint8_t j = 0; j < MAX_LEMONS; ++j) {
                if (lemons[j].active && lemons[j].kind == LEMON_NORMAL) {
                    target_x = lemons[j].x;
                    target_y = lemons[j].y;
                    break;
                }
            }
        }
        int speed = 1 + (state == STATE_ESCAPE) + (screen_no > 3 && ((frame_no + i) & 1u));
        if (target_x > c->x) {
            c->x += speed;
            c->facing = 1;
        } else if (target_x < c->x) {
            c->x -= speed;
            c->facing = -1;
        }
        if (target_y > c->y) c->y += speed; else if (target_y < c->y) c->y -= speed;
        c->x = (int16_t)clamp_i(c->x, 4, SCREEN_W - CHICKEN_W - 4);
        c->y = (int16_t)clamp_i(c->y, 24, SCREEN_H - CHICKEN_H - 4);

        for (uint8_t j = 0; j < MAX_LEMONS; ++j) {
            Lemon *l = &lemons[j];
            if (l->active && l->kind == LEMON_NORMAL && overlap(c->x, c->y, CHICKEN_W, CHICKEN_H, l->x, l->y, LEMON_W, LEMON_H)) {
                l->active = 0;
                if (score > 3) score -= 3;
                prg32_audio_beep(180, 18);
            }
        }
    }
}

static void update_rooster(void) {
    if (!rooster_enabled_for_screen(screen_no)) {
        rooster.active = 0;
        rooster.scared = 0;
        return;
    }

    if (!rooster.active) {
        if (rooster.cooldown > 0) rooster.cooldown--;
        if (rooster.cooldown == 0 && ((rnd() & 31u) == 0u || state == STATE_ESCAPE)) {
            rooster.active = 1;
            rooster.scared = 0;
            rooster.x = (rnd() & 1u) ? 2 : SCREEN_W - ROOSTER_W - 2;
            rooster.y = (int16_t)(34 + (rnd() % 130u));
            rooster.facing = (rooster.x < SCREEN_W / 2) ? 1 : -1;
            prg32_audio_beep(196, 120);
        }
        return;
    }

    int speed = rooster.scared ? 4 : (state == STATE_ESCAPE ? 3 : 2);
    int target_x = rooster.scared ? ((rooster.x < SCREEN_W / 2) ? -24 : SCREEN_W + 24) : moana_x;
    int target_y = rooster.scared ? rooster.y : moana_y;
    if (target_x > rooster.x) {
        rooster.x += speed;
        rooster.facing = 1;
    } else if (target_x < rooster.x) {
        rooster.x -= speed;
        rooster.facing = -1;
    }
    if (target_y > rooster.y) rooster.y += speed; else if (target_y < rooster.y) rooster.y -= speed;
    if (rooster.x < -20 || rooster.x > SCREEN_W + 20) {
        rooster.active = 0;
        rooster.scared = 0;
        rooster.cooldown = (uint16_t)(180 + (rnd() % 180u));
    }
}

static void hurt_moana(const char *text) {
    (void)text;
    hurt_timer = 60;
    trigger_effect(EFFECT_HURT, 60);
    if (basket > 0) basket--;
    screen_timer = (screen_timer > 90) ? screen_timer - 90 : 1;
    prg32_audio_beep(98, 150);
    moana_x = 152;
    moana_y = 106;
}

static void check_hazards(void) {
    if (hurt_timer) return;
    if (rooster.active && !rooster.scared && overlap(moana_x, moana_y, PLAYER_W, PLAYER_H, rooster.x, rooster.y, ROOSTER_W, ROOSTER_H)) {
        hurt_moana("Rooster attack!");
    }
}

static void check_escape(void) {
    if (state != STATE_ESCAPE) return;
    if (overlap(moana_x, moana_y, PLAYER_W, PLAYER_H, door_x, door_y, 16, 16)) {
        score += (uint16_t)(250 + screen_timer / FPS);
        if (screen_no >= FINAL_LEVEL) {
            state = STATE_FINAL_VICTORY;
        } else {
            next_screen_after_clear = (uint8_t)(screen_no + 1u);
            state = STATE_LEVEL_CLEAR;
        }
        prg32_audio_beep(1047, 140);
    }
}

static void draw_tree(int x, int y, uint8_t phase) {
    prg32_gfx_rect(x - 3, y + 24, 7, 32, COLOR_TRUNK);
    prg32_gfx_rect(x - 18, y + 8, 36, 22, COLOR_TREE);
    prg32_gfx_rect(x - 12, y, 24, 34, 0x2cc4);
    prg32_gfx_rect(x - 24, y + 18, 48, 12, 0x2d66);
    for (uint8_t i = 0; i < 5; ++i) {
        int lx = x - 15 + i * 7;
        int ly = y + 10 + ((phase + i * 3) & 7u);
        prg32_gfx_rect(lx, ly, 5, 5, COLOR_LEMON);
    }
}

static void draw_field(void) {
    prg32_gfx_clear(COLOR_SKY);
    prg32_gfx_rect(0, 0, 320, 18, 0x047f);
    prg32_gfx_rect(0, 18, 320, 12, 0x7e9f);
    prg32_gfx_rect(132, 54, 72, 12, 0xbdf7);
    prg32_gfx_rect(140, 42, 50, 12, 0xc638);
    prg32_gfx_rect(148, 30, 12, 12, 0xa514);
    prg32_gfx_rect(174, 28, 12, 14, 0xa514);
    prg32_gfx_rect(188, 36, 10, 18, 0x9cd3);
    prg32_gfx_rect(126, 66, 86, 8, 0x7bce);
    prg32_gfx_rect(112, 74, 118, 8, COLOR_SEA);
    prg32_gfx_rect(0, 142, 320, 58, COLOR_GRASS);
    for (int x = 0; x < SCREEN_W; x += 16) {
        prg32_gfx_rect(x, 176 + ((x >> 4) & 3), 12, 2, COLOR_GRASS_DK);
    }
    prg32_gfx_rect(0, 188, 320, 12, COLOR_SAND);
    for (uint8_t i = 0; i < tree_count_for_screen(screen_no); ++i) draw_tree(tree_x[i], tree_y[i], (uint8_t)(frame_no + i * 5u));
}

static void draw_splash_frame(uint16_t panel, uint16_t inner) {
    draw_field();
    prg32_gfx_rect(22, 32, 276, 118, 0x0149);
    prg32_gfx_rect(24, 34, 272, 114, panel);
    prg32_gfx_rect(28, 38, 264, 106, inner);
}

static uint16_t lemon_color(LemonKind kind) {
    if (kind == LEMON_MAGIC) return COLOR_MAGIC;
    if (kind == LEMON_ENERGY) return COLOR_ENERGY;
    if (kind == LEMON_BAD) return COLOR_BAD;
    return COLOR_LEMON;
}

static void draw_lemon(const Lemon *l) {
    uint16_t c = lemon_color(l->kind);
    prg32_gfx_rect(l->x + 8, l->y + 5, 8, 14, c);
    prg32_gfx_rect(l->x + 5, l->y + 8, 14, 8, c);
    prg32_gfx_rect(l->x + 14, l->y + 4, 6, 3, 0x2cc4);
    if (l->kind == LEMON_MAGIC) prg32_gfx_rect(l->x + 10, l->y + 10, 4, 4, PRG32_COLOR_WHITE);
    if (l->kind == LEMON_ENERGY) prg32_gfx_rect(l->x + 11, l->y + 6, 3, 12, PRG32_COLOR_WHITE);
    if (l->age + 25 > l->ttl) prg32_gfx_rect(l->x + 8, l->y + 12, 9, 3, PRG32_COLOR_BLACK);
}

static void draw_moana_pose(int x, int y, int8_t facing, uint8_t fx_timer, EffectKind fx_kind) {
    uint8_t anim = (uint8_t)((frame_no >> 3) & 3u);
    int face_left = facing < 0;
    int hair_side = face_left ? 17 : 2;
    int front_arm = face_left ? 1 : 18;
    int back_arm = face_left ? 18 : 1;
    int basket_x = face_left ? 1 : 17;
    int lemon_x = face_left ? 2 : 18;
    prg32_gfx_rect(x + 4, y, 15, 8, COLOR_HAIR);
    prg32_gfx_rect(x + hair_side, y + 4, 5, 6, COLOR_HAIR);
    prg32_gfx_rect(x + 6, y + 4, 12, 9, COLOR_SKIN);
    prg32_gfx_rect(x + 8, y + 7, 2, 2, COLOR_EYE);
    prg32_gfx_rect(x + 15, y + 7, 2, 2, COLOR_EYE);
    prg32_gfx_rect(x + 10, y + 11, 5, 1, 0xdaa8);
    prg32_gfx_rect(x + 3, y + 13, 18, 7, COLOR_DRESS);
    prg32_gfx_rect(x + back_arm, y + 15, 5, 5, COLOR_SKIN);
    prg32_gfx_rect(x + front_arm, y + 15, 5, 5, COLOR_SKIN);
    prg32_gfx_rect(x + 6, y + 20, 4, anim & 1u ? 3 : 4, COLOR_SHADOW);
    prg32_gfx_rect(x + 14, y + 20, 4, anim & 1u ? 4 : 3, COLOR_SHADOW);
    prg32_gfx_rect(x + basket_x, y + 9, 6, 7, COLOR_TRUNK);
    prg32_gfx_rect(x + lemon_x, y + 10, 4, 4, COLOR_LEMON);
    if (fx_timer) {
        uint8_t pulse = (uint8_t)((frame_no >> 2) & 3u);
        if (fx_kind == EFFECT_MAGIC) {
            prg32_gfx_rect(x + 1 + pulse, y - 3, 5, 5, COLOR_MAGIC);
            prg32_gfx_rect(x + 18 - pulse, y - 4, 4, 4, PRG32_COLOR_WHITE);
        } else if (fx_kind == EFFECT_ENERGY) {
            prg32_gfx_rect(x - 2, y + 3 + pulse, 2, 12, COLOR_ENERGY);
            prg32_gfx_rect(x + 24, y + 5 - pulse, 2, 12, COLOR_ENERGY);
        } else if (fx_kind == EFFECT_BAD) {
            prg32_gfx_rect(x + 5, y + 2, 14, 3, COLOR_BAD);
            prg32_gfx_rect(x + 6 + pulse, y + 15, 12, 3, COLOR_BAD);
        } else if (fx_kind == EFFECT_HURT) {
            prg32_gfx_rect(x + 2, y + 1, 20, 3, PRG32_COLOR_RED);
            prg32_gfx_rect(x + 2, y + 19, 20, 3, PRG32_COLOR_RED);
        }
    }
}

static void draw_moana(void) {
    draw_moana_pose(moana_x, moana_y, moana_facing, effect_timer, effect_kind);
}

static void draw_chicken(const Chicken *c, uint8_t i) {
    uint8_t flap = (uint8_t)(((frame_no >> 3) + i) & 1u);
    int face_left = c->facing < 0;
    int head_x = face_left ? 2 : 14;
    int beak_x = face_left ? 0 : 21;
    int eye_x = face_left ? 5 : 17;
    int wing_x = face_left ? (flap ? 15 : 13) : (flap ? 1 : 3);
    prg32_gfx_rect(c->x + 5, c->y + 7, 14, 10, COLOR_CHICKEN);
    prg32_gfx_rect(c->x + head_x, c->y + 3, 8, 8, COLOR_CHICKEN);
    prg32_gfx_rect(c->x + beak_x, c->y + 7, 3, 3, COLOR_ENERGY);
    prg32_gfx_rect(c->x + eye_x, c->y + 6, 2, 2, PRG32_COLOR_BLACK);
    prg32_gfx_rect(c->x + wing_x, c->y + 10, 8, 5, PRG32_COLOR_WHITE);
    prg32_gfx_rect(c->x + 8, c->y + 17, 3, 6, COLOR_ENERGY);
    prg32_gfx_rect(c->x + 15, c->y + 17, 3, 6, COLOR_ENERGY);
}

static void draw_rooster(void) {
    if (!rooster.active) return;
    uint16_t body = rooster.scared ? 0x7bef : COLOR_ROOSTER;
    int face_left = rooster.facing < 0;
    int head_x = face_left ? 2 : 13;
    int comb_x = face_left ? 3 : 14;
    int beak_x = face_left ? 0 : 21;
    int eye_x = face_left ? 6 : 17;
    int tail_x = face_left ? 16 : 0;
    prg32_gfx_rect(rooster.x + 4, rooster.y + 8, 15, 10, body);
    prg32_gfx_rect(rooster.x + head_x, rooster.y + 2, 9, 9, body);
    prg32_gfx_rect(rooster.x + comb_x, rooster.y, 7, 3, COLOR_ENERGY);
    prg32_gfx_rect(rooster.x + beak_x, rooster.y + 6, 3, 3, COLOR_LEMON);
    prg32_gfx_rect(rooster.x + eye_x, rooster.y + 5, 2, 2, PRG32_COLOR_BLACK);
    prg32_gfx_rect(rooster.x + tail_x, rooster.y + 10, 8, 7, 0x001f);
    prg32_gfx_rect(rooster.x + 7, rooster.y + 18, 3, 6, COLOR_ENERGY);
    prg32_gfx_rect(rooster.x + 15, rooster.y + 18, 3, 6, COLOR_ENERGY);
}

static void draw_hud(void) {
    uint16_t secs = (uint16_t)(screen_timer / FPS);
    prg32_gfx_rect(0, 0, SCREEN_W, 16, PRG32_COLOR_BLACK);
    prg32_gfx_text8(4, 4, "L", PRG32_COLOR_WHITE, PRG32_COLOR_BLACK);
    draw_num2(18, 4, screen_no, COLOR_MAGIC);
    prg32_gfx_text8(42, 4, "T", PRG32_COLOR_WHITE, PRG32_COLOR_BLACK);
    draw_num3(56, 4, secs, secs < 20 ? PRG32_COLOR_RED : PRG32_COLOR_WHITE);
    prg32_gfx_text8(92, 4, "B", PRG32_COLOR_WHITE, PRG32_COLOR_BLACK);
    draw_num2(106, 4, basket, COLOR_LEMON);
    prg32_gfx_text8(132, 4, "Q", PRG32_COLOR_WHITE, PRG32_COLOR_BLACK);
    draw_num2(146, 4, collected_this_screen, COLOR_LEMON);
    prg32_gfx_text8(166, 4, "/", PRG32_COLOR_WHITE, PRG32_COLOR_BLACK);
    draw_num2(174, 4, quota, COLOR_LEMON);
    prg32_gfx_text8(210, 4, "S", PRG32_COLOR_WHITE, PRG32_COLOR_BLACK);
    draw_num5(224, 4, score, PRG32_COLOR_WHITE);
}

static void draw_door(void) {
    if (state != STATE_ESCAPE) return;
    prg32_gfx_rect(door_x, door_y, 24, 24, COLOR_DOOR);
    prg32_gfx_rect(door_x + 4, door_y + 4, 16, 16, COLOR_DOOR_LIT);
    prg32_gfx_rect(door_x + 17, door_y + 12, 3, 3, PRG32_COLOR_BLACK);
}

static void draw_splash(void) {
    draw_splash_frame(COLOR_LEMON, 0x0228);
    prg32_gfx_text8(58, 52, "MOANA AND THE", PRG32_COLOR_WHITE, 0x0228);
    prg32_gfx_text8(44, 70, "LEMON APOCALYPSE", COLOR_LEMON, 0x0228);
    prg32_gfx_rect(68, 94, 184, 16, COLOR_TREE);
    prg32_gfx_text8(78, 98, "SELECT: start quest", PRG32_COLOR_WHITE, COLOR_TREE);
    prg32_gfx_text8(76, 118, "A throws basket lemons", COLOR_MAGIC, 0x0228);
    prg32_gfx_text8(48, 164, "Castello Aragonese edition", PRG32_COLOR_BLACK, COLOR_SAND);
    draw_tree(66, 118, (uint8_t)frame_no);
    draw_tree(250, 116, (uint8_t)(frame_no + 4u));
    draw_moana_pose(150, 126, 1, 0, EFFECT_NONE);
}

static void draw_level_clear(void) {
    draw_splash_frame(COLOR_LEMON, 0x0248);
    prg32_gfx_text8(82, 54, "LEMON ESCAPE!", COLOR_LEMON, 0x0248);
    prg32_gfx_text8(72, 78, "Moana found the door", PRG32_COLOR_WHITE, 0x0248);
    prg32_gfx_text8(74, 100, "SELECT: next level", COLOR_MAGIC, 0x0248);
    draw_tree(66, 116, (uint8_t)(frame_no + 2u));
    draw_moana_pose(148, 120, 1, 24, EFFECT_MAGIC);
}

static void draw_game_over_splash(void) {
    draw_splash_frame(0x7a86, 0x2104);
    prg32_gfx_text8(100, 54, "NOT TODAY", COLOR_LEMON, 0x2104);
    prg32_gfx_text8(62, 78, "Moana is sad, but ready", PRG32_COLOR_WHITE, 0x2104);
    prg32_gfx_text8(74, 96, "to try one more time.", PRG32_COLOR_WHITE, 0x2104);
    prg32_gfx_text8(74, 120, "SELECT: rise again", COLOR_MAGIC, 0x2104);
    draw_moana_pose(148, 124, -1, 0, EFFECT_NONE);
    prg32_gfx_rect(156, 134, 8, 2, COLOR_BAD);
}

static void draw_final_victory(void) {
    draw_splash_frame(0xffe0, 0x0348);
    prg32_gfx_text8(70, 50, "GLORIOUS LEMONS!", COLOR_LEMON, 0x0348);
    prg32_gfx_text8(58, 74, "Moana saved the grove", PRG32_COLOR_WHITE, 0x0348);
    prg32_gfx_text8(78, 96, "and outran them all.", PRG32_COLOR_WHITE, 0x0348);
    prg32_gfx_text8(78, 122, "SELECT: play again", COLOR_MAGIC, 0x0348);
    draw_tree(58, 118, (uint8_t)frame_no);
    draw_tree(246, 118, (uint8_t)(frame_no + 6u));
    draw_moana_pose(148, 120, 1, 24, EFFECT_MAGIC);
}

void moana_lemon_c_init(void) {
    state = STATE_SPLASH;
    rng_state = 0x4d6f616eu;
    frame_no = 0;
    prev_input = 0;
    moana_facing = 1;
    moana_x = 150;
    moana_y = 126;
}

void moana_lemon_c_update(void) {
    frame_no++;
    uint32_t input = prg32_input_read();
    uint32_t pressed = input & ~prev_input;

    if (state == STATE_SPLASH) {
        play_melody_tick();
        if (pressed & BTN_SELECT) start_new_game();
        prev_input = input;
        return;
    }
    if (state == STATE_LEVEL_CLEAR) {
        play_victory_sound_tick();
        if (pressed & BTN_SELECT) start_screen(next_screen_after_clear);
        prev_input = input;
        return;
    }
    if (state == STATE_GAME_OVER) {
        if ((frame_no % 48u) == 0u) prg32_audio_beep(165, 80);
        if (pressed & BTN_SELECT) start_new_game();
        prev_input = input;
        return;
    }
    if (state == STATE_FINAL_VICTORY) {
        play_final_song_tick();
        if (pressed & BTN_SELECT) start_new_game();
        prev_input = input;
        return;
    }

    play_melody_tick();
    if (screen_timer > 0) screen_timer--;
    if (screen_timer == 0) {
        state = STATE_GAME_OVER;
        prg32_audio_beep(80, 240);
        prev_input = input;
        return;
    }

    if (pressed & BTN_THROW) throw_lemon();
    update_moana(input & BTN_MOVE_MASK);
    update_lemons();
    update_shots();
    update_chickens();
    update_rooster();
    check_hazards();
    check_escape();
    prev_input = input;
}

void moana_lemon_c_draw(void) {
    if (state == STATE_SPLASH) {
        draw_splash();
        return;
    }
    if (state == STATE_LEVEL_CLEAR) {
        draw_level_clear();
        return;
    }
    if (state == STATE_GAME_OVER) {
        draw_game_over_splash();
        return;
    }
    if (state == STATE_FINAL_VICTORY) {
        draw_final_victory();
        return;
    }

    draw_field();
    draw_door();
    for (uint8_t i = 0; i < MAX_LEMONS; ++i) if (lemons[i].active) draw_lemon(&lemons[i]);
    for (uint8_t i = 0; i < MAX_SHOTS; ++i) {
        if (shots[i].active) {
            prg32_gfx_rect(shots[i].x + 1, shots[i].y, 4, 6, COLOR_LEMON);
            prg32_gfx_rect(shots[i].x, shots[i].y + 2, 6, 2, COLOR_LEMON);
        }
    }
    for (uint8_t i = 0; i < chicken_count_for_screen(screen_no); ++i) draw_chicken(&chickens[i], i);
    draw_rooster();
    draw_moana();
    draw_hud();
}
