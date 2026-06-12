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
#define NUM_TREES 5
#define MAX_LEMONS 16
#define NUM_CHICKENS 4
#define MAX_SHOTS 3
#define FPS 30
#define SCREEN_TIME_FRAMES (150u * FPS)
#define MAGIC_TIME_FRAMES (30u * FPS)
#define BAD_TIME_FRAMES (15u * FPS)
#define ENERGY_FRAMES (12u * FPS)
#define QUOTA_BASE 18

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
#define BTN_MOVE_MASK (PRG32_BTN_LEFT | PRG32_BTN_RIGHT | PRG32_BTN_UP | PRG32_BTN_DOWN)

typedef enum {
    STATE_SPLASH = 0,
    STATE_PLAY = 1,
    STATE_ESCAPE = 2,
    STATE_GAME_OVER = 3
} GameState;

typedef enum {
    LEMON_NORMAL = 0,
    LEMON_MAGIC = 1,
    LEMON_ENERGY = 2,
    LEMON_BAD = 3
} LemonKind;

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
} Chicken;

typedef struct {
    int16_t x;
    int16_t y;
    int8_t active;
    int8_t scared;
    uint16_t cooldown;
} Rooster;

static const int16_t tree_x[NUM_TREES] = { 36, 98, 160, 226, 284 };
static const int16_t tree_y[NUM_TREES] = { 42, 28, 48, 32, 50 };
static const uint16_t melody[] = {
    392, 392, 440, 392, 330, 0, 330, 349, 392, 349, 330, 0,
    392, 392, 440, 392, 330, 0, 294, 330, 349, 330, 294, 0
};

static GameState state;
static Lemon lemons[MAX_LEMONS];
static Shot shots[MAX_SHOTS];
static Chicken chickens[NUM_CHICKENS];
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
static uint8_t last_dir;
static int16_t moana_x;
static int16_t moana_y;
static int16_t door_x;
static int16_t door_y;
static uint8_t door_side;
static uint8_t message_timer;
static const char *message_text;

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
    if ((frame_no % 12u) == 0u) {
        uint16_t note = melody[(frame_no / 12u) % (sizeof(melody) / sizeof(melody[0]))];
        if (note) prg32_audio_beep(note, 38);
    }
}

static void reset_chickens(void) {
    for (uint8_t i = 0; i < NUM_CHICKENS; ++i) {
        chickens[i].x = (int16_t)(24 + i * 72);
        chickens[i].y = (int16_t)(150 + ((i & 1u) ? 12 : 0));
        chickens[i].dx = (i & 1u) ? 1 : -1;
        chickens[i].dy = (i & 2u) ? 1 : -1;
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
    screen_no = next_screen;
    quota = (uint8_t)(QUOTA_BASE + (screen_no - 1u) * 4u);
    collected_this_screen = 0;
    screen_timer = SCREEN_TIME_FRAMES;
    energy_timer = 0;
    message_text = "Gather lemons!";
    message_timer = 80;
    state = STATE_PLAY;
}

static void start_new_game(void) {
    rng_state = 0x4d6f616eu;
    frame_no = 0;
    score = 0;
    basket = 0;
    start_screen(1);
}

static LemonKind pick_lemon_kind(void) {
    uint32_t r = rnd() % 100u;
    if (r < 6u) return LEMON_MAGIC;
    if (r < 13u) return LEMON_ENERGY;
    if (r < 21u) return LEMON_BAD;
    return LEMON_NORMAL;
}

static void spawn_lemon(void) {
    for (uint8_t i = 0; i < MAX_LEMONS; ++i) {
        Lemon *l = &lemons[i];
        if (!l->active) {
            uint8_t tree = (uint8_t)(rnd() % NUM_TREES);
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

static void update_lemons(void) {
    uint8_t spawn_every = (uint8_t)(36 - clamp_i(screen_no * 3, 0, 18));
    if ((frame_no % spawn_every) == 0u) spawn_lemon();

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
        message_text = "Bad lemon!";
        message_timer = 50;
        prg32_audio_beep(110, 120);
        return;
    }
    if (l->kind == LEMON_MAGIC) {
        screen_timer += MAGIC_TIME_FRAMES;
        score += 75;
        message_text = "+30 seconds";
        message_timer = 50;
        prg32_audio_beep(880, 90);
    } else if (l->kind == LEMON_ENERGY) {
        energy_timer = 255;
        score += 50;
        message_text = "Energy!";
        message_timer = 50;
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
        message_text = "Escape!";
        message_timer = 90;
        prg32_audio_beep(988, 140);
    }
}

static void update_moana(uint32_t input) {
    int speed = energy_timer ? 4 : 3;
    if (energy_timer) energy_timer--;
    if (input & PRG32_BTN_LEFT) {
        moana_x -= speed;
        last_dir = 3;
    }
    if (input & PRG32_BTN_RIGHT) {
        moana_x += speed;
        last_dir = 1;
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
            message_text = "Rooster scared!";
            message_timer = 55;
            prg32_audio_beep(1047, 120);
        }
    }
}

static void update_chickens(void) {
    for (uint8_t i = 0; i < NUM_CHICKENS; ++i) {
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
        int speed = 1 + (state == STATE_ESCAPE) + (screen_no > 2 && ((frame_no + i) & 1u));
        if (target_x > c->x) c->x += speed; else if (target_x < c->x) c->x -= speed;
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
    if (!rooster.active) {
        if (rooster.cooldown > 0) rooster.cooldown--;
        if (rooster.cooldown == 0 && ((rnd() & 31u) == 0u || state == STATE_ESCAPE)) {
            rooster.active = 1;
            rooster.scared = 0;
            rooster.x = (rnd() & 1u) ? 2 : SCREEN_W - ROOSTER_W - 2;
            rooster.y = (int16_t)(34 + (rnd() % 130u));
            prg32_audio_beep(196, 120);
        }
        return;
    }

    int speed = rooster.scared ? 4 : (state == STATE_ESCAPE ? 3 : 2);
    int target_x = rooster.scared ? ((rooster.x < SCREEN_W / 2) ? -24 : SCREEN_W + 24) : moana_x;
    int target_y = rooster.scared ? rooster.y : moana_y;
    if (target_x > rooster.x) rooster.x += speed; else if (target_x < rooster.x) rooster.x -= speed;
    if (target_y > rooster.y) rooster.y += speed; else if (target_y < rooster.y) rooster.y -= speed;
    if (rooster.x < -20 || rooster.x > SCREEN_W + 20) {
        rooster.active = 0;
        rooster.scared = 0;
        rooster.cooldown = (uint16_t)(180 + (rnd() % 180u));
    }
}

static void hurt_moana(const char *text) {
    if (basket > 0) basket--;
    screen_timer = (screen_timer > 90) ? screen_timer - 90 : 1;
    message_text = text;
    message_timer = 55;
    prg32_audio_beep(98, 150);
    moana_x = 152;
    moana_y = 106;
}

static void check_hazards(void) {
    for (uint8_t i = 0; i < NUM_CHICKENS; ++i) {
        if (overlap(moana_x, moana_y, PLAYER_W, PLAYER_H, chickens[i].x, chickens[i].y, CHICKEN_W, CHICKEN_H)) {
            hurt_moana("Chicken bump!");
            return;
        }
    }
    if (rooster.active && !rooster.scared && overlap(moana_x, moana_y, PLAYER_W, PLAYER_H, rooster.x, rooster.y, ROOSTER_W, ROOSTER_H)) {
        hurt_moana("Rooster attack!");
    }
}

static void check_escape(void) {
    if (state != STATE_ESCAPE) return;
    if (overlap(moana_x, moana_y, PLAYER_W, PLAYER_H, door_x, door_y, 16, 16)) {
        score += (uint16_t)(250 + screen_timer / FPS);
        start_screen(screen_no + 1u);
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
    for (uint8_t i = 0; i < NUM_TREES; ++i) draw_tree(tree_x[i], tree_y[i], (uint8_t)(frame_no + i * 5u));
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

static void draw_moana(void) {
    uint8_t anim = (uint8_t)((frame_no >> 3) & 3u);
    int x = moana_x;
    int y = moana_y;
    prg32_gfx_rect(x + 4, y, 15, 8, COLOR_HAIR);
    prg32_gfx_rect(x + 2, y + 4, 5, 6, COLOR_HAIR);
    prg32_gfx_rect(x + 6, y + 4, 12, 9, COLOR_SKIN);
    prg32_gfx_rect(x + 8, y + 7, 2, 2, COLOR_EYE);
    prg32_gfx_rect(x + 15, y + 7, 2, 2, COLOR_EYE);
    prg32_gfx_rect(x + 10, y + 11, 5, 1, 0xdaa8);
    prg32_gfx_rect(x + 3, y + 13, 18, 7, COLOR_DRESS);
    prg32_gfx_rect(x + 1, y + 15, 5, 5, COLOR_SKIN);
    prg32_gfx_rect(x + 18, y + 15, 5, 5, COLOR_SKIN);
    prg32_gfx_rect(x + 6, y + 20, 4, anim & 1u ? 3 : 4, COLOR_SHADOW);
    prg32_gfx_rect(x + 14, y + 20, 4, anim & 1u ? 4 : 3, COLOR_SHADOW);
    prg32_gfx_rect(x + 17, y + 9, 6, 7, COLOR_TRUNK);
    prg32_gfx_rect(x + 18, y + 10, 4, 4, COLOR_LEMON);
}

static void draw_chicken(const Chicken *c, uint8_t i) {
    uint8_t flap = (uint8_t)(((frame_no >> 3) + i) & 1u);
    prg32_gfx_rect(c->x + 5, c->y + 7, 14, 10, COLOR_CHICKEN);
    prg32_gfx_rect(c->x + 14, c->y + 3, 8, 8, COLOR_CHICKEN);
    prg32_gfx_rect(c->x + 21, c->y + 7, 3, 3, COLOR_ENERGY);
    prg32_gfx_rect(c->x + 17, c->y + 6, 2, 2, PRG32_COLOR_BLACK);
    prg32_gfx_rect(c->x + (flap ? 1 : 3), c->y + 10, 8, 5, PRG32_COLOR_WHITE);
    prg32_gfx_rect(c->x + 8, c->y + 17, 3, 6, COLOR_ENERGY);
    prg32_gfx_rect(c->x + 15, c->y + 17, 3, 6, COLOR_ENERGY);
}

static void draw_rooster(void) {
    if (!rooster.active) return;
    uint16_t body = rooster.scared ? 0x7bef : COLOR_ROOSTER;
    prg32_gfx_rect(rooster.x + 4, rooster.y + 8, 15, 10, body);
    prg32_gfx_rect(rooster.x + 13, rooster.y + 2, 9, 9, body);
    prg32_gfx_rect(rooster.x + 14, rooster.y, 7, 3, COLOR_ENERGY);
    prg32_gfx_rect(rooster.x + 21, rooster.y + 6, 3, 3, COLOR_LEMON);
    prg32_gfx_rect(rooster.x + 17, rooster.y + 5, 2, 2, PRG32_COLOR_BLACK);
    prg32_gfx_rect(rooster.x, rooster.y + 10, 8, 7, 0x001f);
    prg32_gfx_rect(rooster.x + 7, rooster.y + 18, 3, 6, COLOR_ENERGY);
    prg32_gfx_rect(rooster.x + 15, rooster.y + 18, 3, 6, COLOR_ENERGY);
}

static void draw_hud(void) {
    uint16_t secs = (uint16_t)(screen_timer / FPS);
    prg32_gfx_rect(0, 0, 320, 16, PRG32_COLOR_BLACK);
    prg32_gfx_text8(4, 4, "MOANA LEMON", COLOR_LEMON, PRG32_COLOR_BLACK);
    prg32_gfx_text8(104, 4, "T", PRG32_COLOR_WHITE, PRG32_COLOR_BLACK);
    draw_num3(120, 4, secs, secs < 20 ? PRG32_COLOR_RED : PRG32_COLOR_WHITE);
    prg32_gfx_text8(158, 4, "B", PRG32_COLOR_WHITE, PRG32_COLOR_BLACK);
    draw_num2(174, 4, basket, COLOR_LEMON);
    prg32_gfx_text8(210, 4, "Q", PRG32_COLOR_WHITE, PRG32_COLOR_BLACK);
    draw_num2(226, 4, collected_this_screen, COLOR_LEMON);
    prg32_gfx_text8(246, 4, "/", PRG32_COLOR_WHITE, PRG32_COLOR_BLACK);
    draw_num2(258, 4, quota, COLOR_LEMON);
    draw_num5(282, 4, score, PRG32_COLOR_WHITE);
}

static void draw_door(void) {
    if (state != STATE_ESCAPE) return;
    prg32_gfx_rect(door_x, door_y, 24, 24, COLOR_DOOR);
    prg32_gfx_rect(door_x + 4, door_y + 4, 16, 16, COLOR_DOOR_LIT);
    prg32_gfx_rect(door_x + 17, door_y + 12, 3, 3, PRG32_COLOR_BLACK);
}

static void draw_message(void) {
    if (!message_timer || !message_text) return;
    prg32_gfx_rect(86, 78, 148, 24, PRG32_COLOR_BLACK);
    prg32_gfx_rect(88, 80, 144, 20, 0x2144);
    prg32_gfx_text8(102, 86, message_text, COLOR_LEMON, 0x2144);
}

static void draw_splash(void) {
    draw_field();
    prg32_gfx_rect(22, 32, 276, 118, 0x0149);
    prg32_gfx_rect(24, 34, 272, 114, COLOR_LEMON);
    prg32_gfx_rect(28, 38, 264, 106, 0x0228);
    prg32_gfx_text8(58, 52, "MOANA AND THE", PRG32_COLOR_WHITE, 0x0228);
    prg32_gfx_text8(44, 70, "LEMON APOCALYPSE", COLOR_LEMON, 0x0228);
    prg32_gfx_text8(54, 100, "START: gather, throw, escape", PRG32_COLOR_WHITE, 0x0228);
    prg32_gfx_text8(76, 118, "A throws basket lemons", COLOR_MAGIC, 0x0228);
    prg32_gfx_text8(48, 164, "Castello Aragonese edition", PRG32_COLOR_BLACK, COLOR_SAND);
    draw_moana();
}

void moana_lemon_c_init(void) {
    state = STATE_SPLASH;
    rng_state = 0x4d6f616eu;
    frame_no = 0;
    moana_x = 150;
    moana_y = 126;
}

void moana_lemon_c_update(void) {
    frame_no++;
    uint32_t input = prg32_input_read();
    play_melody_tick();

    if (state == STATE_SPLASH) {
        if (input & PRG32_BTN_START) start_new_game();
        return;
    }
    if (state == STATE_GAME_OVER) {
        if (input & PRG32_BTN_START) start_new_game();
        return;
    }

    if (message_timer) message_timer--;
    if (screen_timer > 0) screen_timer--;
    if (screen_timer == 0) {
        state = STATE_GAME_OVER;
        message_text = "Time over!";
        message_timer = 255;
        prg32_audio_beep(80, 240);
        return;
    }

    if ((input & BTN_THROW) && (frame_no & 7u) == 0u) throw_lemon();
    update_moana(input & BTN_MOVE_MASK);
    update_lemons();
    update_shots();
    update_chickens();
    update_rooster();
    check_hazards();
    check_escape();
}

void moana_lemon_c_draw(void) {
    if (state == STATE_SPLASH) {
        draw_splash();
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
    for (uint8_t i = 0; i < NUM_CHICKENS; ++i) draw_chicken(&chickens[i], i);
    draw_rooster();
    draw_moana();
    draw_hud();
    draw_message();

    if (state == STATE_GAME_OVER) {
        prg32_gfx_rect(54, 68, 212, 56, PRG32_COLOR_BLACK);
        prg32_gfx_text8(110, 78, "GAME OVER", PRG32_COLOR_RED, PRG32_COLOR_BLACK);
        prg32_gfx_text8(72, 96, "Press START for more lemons", COLOR_LEMON, PRG32_COLOR_BLACK);
    }
}
