//*****************************************************************************
//
// game.c - Mini Doom: raycaster game on the BoosterPack MKII display (128x128)
//
// Rendering (like Wolfenstein 3D):
//   For every one of the 128 screen columns a ray is sent from the player
//   into the 16x16 tile map (DDA = step from grid line to grid line) until
//   it hits a wall. Wall height on screen = VIEW_H / distance.
//   The distance of every column is kept in zbuf[], so enemies (sprites)
//   are only drawn where they are nearer than the wall.
//
// Map: '.' floor, '1' '2' '3' walls, 'D' closed door, 'O' open door.
// World coordinates: x = column (to the right), y = row (downwards),
// 1.0 = one tile.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "grlib/grlib.h"
#include "lcd/Crystalfontz128x128_ST7735.h"

#include "melodies.h"
#include "sound.h"
#include "game.h"

#define MAP_W         16
#define MAP_H         16
#define SCREEN_W     128
#define VIEW_H       112      // 3D view: rows 0 .. 111
#define HUD_Y        112      // status bar: rows 112 .. 127

#define ENEMY_COUNT    5
#define ENEMY_HP       2      // hits until an enemy dies
#define MOVE_SPEED  2.6f      // tiles per second
#define TURN_SPEED  2.4f      // radians per second
#define PLAYER_R    0.22f     // collision radius of the player
#define ENEMY_SPEED 1.3f
#define ENEMY_R     0.25f
#define ATTACK_DIST 0.9f      // enemies hit the player below this distance
#define ATTACK_MS    900      // time between two enemy hits
#define ATTACK_DMG    12
#define SHOOT_MS     280      // fire rate of the gun
#define DEADZONE     350      // joystick: ignore small deflections
#define JOY_Y_SIGN     1      // set to -1 if forward/back are swapped

typedef enum { ST_TITLE, ST_PLAY, ST_DEAD, ST_WIN } GameState;

typedef struct
{
    float    x, y;
    int      hp;              // 0 = dead
    bool     active;          // has seen the player and hunts him
    uint32_t hurtUntil;       // drawn white until this time (hit flash)
    uint32_t nextAttack;
} Enemy;

static const char levelMap[MAP_H][MAP_W + 1] =
{
    "1111111111111111",
    "1......1.......1",
    "1......1.......1",
    "1..22..1.......1",
    "1..22..D...33..1",
    "1......1...33..1",
    "1......1.......1",
    "1111D111111D1111",
    "1......1.......1",
    "1......1.......1",
    "1......D.......1",
    "1.222..1..3.3..1",
    "1......1.......1",
    "1......1.......1",
    "1......1.......1",
    "1111111111111111"
};

static const float enemyStart[ENEMY_COUNT][2] =
{
    { 10.5f, 2.5f }, { 3.5f, 9.5f }, { 11.5f, 9.5f }, { 3.5f, 13.5f }, { 12.5f, 13.5f }
};

// enemy picture 8x8: '.' transparent, 1 body, 2 eyes, 3 outline, 4 teeth
static const char *const IMP[8] =
{
    "..3333..",
    ".311113.",
    "31211213",
    "31444413",
    ".311113.",
    "3.1111.3",
    "..1..1..",
    ".33..33."
};
static const char *const CORPSE[8] =
{
    "........", "........", "........", "........",
    "........", "........", "..3113..", ".311113."
};

static tContext ctx;
static char map[MAP_H][MAP_W];          // working copy (doors change)
static Enemy enemies[ENEMY_COUNT];
static float zbuf[SCREEN_W];            // wall distance of every column

static GameState state = ST_TITLE;
static float px, py;                    // player position
static float dirX, dirY;                // view direction (length 1)
static float planeX, planeY;            // camera plane (field of view ~66 deg)
static int   health, kills;
static uint32_t lastShot, flashUntil, hurtUntil, lastTime;
static float walkPhase;                 // gun bobbing while walking

//----------------------------------------------------------------------------
// helpers
//----------------------------------------------------------------------------

static bool solid(float x, float y)
{
    int tx = (int)x;
    int ty = (int)y;
    char t;

    if (x < 0 || y < 0 || tx >= MAP_W || ty >= MAP_H)
    {
        return true;
    }
    t = map[ty][tx];
    return (t != '.') && (t != 'O');
}

// can a round object with radius r stand at (x, y)?
static bool canStand(float x, float y, float r)
{
    return !solid(x - r, y - r) && !solid(x + r, y - r) &&
           !solid(x - r, y + r) && !solid(x + r, y + r);
}

// free line of sight between two points (steps of 0.1 tiles)
static bool lineOfSight(float x0, float y0, float x1, float y1)
{
    float dx = x1 - x0;
    float dy = y1 - y0;
    float dist = sqrtf(dx * dx + dy * dy);
    int   steps = (int)(dist * 10.0f);
    int   i;

    for (i = 1; i < steps; i++)
    {
        float t = (float)i / (float)steps;
        if (solid(x0 + dx * t, y0 + dy * t))
        {
            return false;
        }
    }
    return true;
}

// make a 24-bit colour darker: f = 0 (black) .. 1 (unchanged)
static uint32_t shade(uint32_t c, float f)
{
    uint32_t r, g, b;
    if (f < 0.0f) f = 0.0f;
    if (f > 1.0f) f = 1.0f;
    r = (uint32_t)(((c >> 16) & 0xFF) * f);
    g = (uint32_t)(((c >> 8) & 0xFF) * f);
    b = (uint32_t)((c & 0xFF) * f);
    return (r << 16) | (g << 8) | b;
}

static void fillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t c)
{
    tRectangle r = { x0, y0, x1, y1 };
    GrContextForegroundSet(&ctx, c);
    GrRectFill(&ctx, &r);
}

static void vline(int32_t x, int32_t y0, int32_t y1, uint32_t c)
{
    if (y0 < 0) y0 = 0;
    if (y1 > VIEW_H - 1) y1 = VIEW_H - 1;
    if (x < 0 || x >= SCREEN_W || y0 > y1) return;
    GrContextForegroundSet(&ctx, c);
    GrLineDrawV(&ctx, x, y0, y1);
}

// enemy position in camera space: tX = left/right, tY = depth
static void toCamera(float wx, float wy, float *tX, float *tY)
{
    float sx = wx - px;
    float sy = wy - py;
    float invDet = 1.0f / (planeX * dirY - dirX * planeY);
    *tX = invDet * (dirY * sx - dirX * sy);
    *tY = invDet * (-planeY * sx + planeX * sy);
}

//----------------------------------------------------------------------------
// game logic
//----------------------------------------------------------------------------

static void newGame(uint32_t now)
{
    int x, y, i;

    for (y = 0; y < MAP_H; y++)
    {
        for (x = 0; x < MAP_W; x++)
        {
            map[y][x] = levelMap[y][x];
        }
    }
    for (i = 0; i < ENEMY_COUNT; i++)
    {
        enemies[i].x = enemyStart[i][0];
        enemies[i].y = enemyStart[i][1];
        enemies[i].hp = ENEMY_HP;
        enemies[i].active = false;
        enemies[i].hurtUntil = 0;
        enemies[i].nextAttack = 0;
    }
    px = 1.5f;  py = 5.5f;              // start in the top left room
    dirX = 1.0f;  dirY = 0.0f;          // looking east
    planeX = 0.0f;  planeY = 0.66f;
    health = 100;
    kills = 0;
    lastShot = 0;  flashUntil = 0;  hurtUntil = 0;
    lastTime = now;
    walkPhase = 0.0f;
    state = ST_PLAY;
    sound_play(SND_START);
}

static void shootGun(uint32_t now)
{
    int   i, best = -1;
    float bestDist = 1000.0f;

    lastShot = now;
    flashUntil = now + 80;
    sound_play(SND_SHOOT);

    // hit = nearest living enemy in the middle of the screen, not behind a wall
    for (i = 0; i < ENEMY_COUNT; i++)
    {
        float tX, tY;
        if (enemies[i].hp == 0) continue;
        toCamera(enemies[i].x, enemies[i].y, &tX, &tY);
        if (tY < 0.2f || tY > zbuf[SCREEN_W / 2]) continue;
        if (fabsf(tX / tY) < 0.25f / tY + 0.04f && tY < bestDist)   // ~ half body width
        {
            best = i;
            bestDist = tY;
        }
    }
    if (best >= 0)
    {
        Enemy *e = &enemies[best];
        e->hp--;
        e->active = true;
        e->hurtUntil = now + 120;
        if (e->hp == 0)
        {
            kills++;
            sound_play(SND_KILL);
            if (kills == ENEMY_COUNT)
            {
                state = ST_WIN;
                sound_play(melodies[3]);     // Super Mario fanfare
            }
        }
        else
        {
            sound_play(SND_HIT);
        }
    }
}

static void useDoor(void)
{
    int tx = (int)(px + dirX);
    int ty = (int)(py + dirY);

    if (tx < 0 || ty < 0 || tx >= MAP_W || ty >= MAP_H) return;
    if (map[ty][tx] == 'D')
    {
        map[ty][tx] = 'O';
        sound_play(SND_DOOR);
    }
    else if (map[ty][tx] == 'O' && !((int)px == tx && (int)py == ty))
    {
        map[ty][tx] = 'D';
        sound_play(SND_DOOR);
    }
}

static void updateEnemies(uint32_t now, float dt)
{
    int i;
    for (i = 0; i < ENEMY_COUNT; i++)
    {
        Enemy *e = &enemies[i];
        float dx, dy, dist;

        if (e->hp == 0) continue;
        dx = px - e->x;
        dy = py - e->y;
        dist = sqrtf(dx * dx + dy * dy);

        if (!e->active)
        {
            if (dist < 8.0f && lineOfSight(e->x, e->y, px, py))
            {
                e->active = true;
                e->nextAttack = now + ATTACK_MS;
                sound_play(SND_ALERT);
            }
            continue;
        }

        // walk towards the player (x and y separately -> slides along walls)
        if (dist > 0.7f)
        {
            float step = ENEMY_SPEED * dt;
            float nx = e->x + dx / dist * step;
            float ny = e->y + dy / dist * step;
            if (canStand(nx, e->y, ENEMY_R)) e->x = nx;
            if (canStand(e->x, ny, ENEMY_R)) e->y = ny;
        }

        // attack
        if (dist < ATTACK_DIST && now >= e->nextAttack)
        {
            e->nextAttack = now + ATTACK_MS;
            health -= ATTACK_DMG;
            hurtUntil = now + 150;
            if (health <= 0)
            {
                health = 0;
                state = ST_DEAD;
                sound_play(SND_DEAD);
                return;
            }
            sound_play(SND_HURT);
        }
    }
}

void game_update(uint32_t now, bool shoot, bool use, int32_t joyX, int32_t joyY)
{
    float dt = (now - lastTime) / 1000.0f;
    lastTime = now;
    if (dt > 0.1f) dt = 0.1f;            // no big jumps after a pause

    if (state != ST_PLAY)
    {
        if ((state == ST_TITLE && (shoot || use)) || (state != ST_TITLE && use))
        {
            newGame(now);
        }
        return;
    }

    // turn: stick right = turn right
    if (joyX > DEADZONE || joyX < -DEADZONE)
    {
        float a = (joyX / 2048.0f) * TURN_SPEED * dt;
        float c = cosf(a), s = sinf(a);
        float ox = dirX, opx = planeX;
        dirX = dirX * c - dirY * s;
        dirY = ox * s + dirY * c;
        planeX = planeX * c - planeY * s;
        planeY = opx * s + planeY * c;
    }

    // walk: stick up = forward
    if (joyY > DEADZONE || joyY < -DEADZONE)
    {
        float v = JOY_Y_SIGN * (joyY / 2048.0f) * MOVE_SPEED * dt;
        float nx = px + dirX * v;
        float ny = py + dirY * v;
        if (canStand(nx, py, PLAYER_R)) px = nx;
        if (canStand(px, ny, PLAYER_R)) py = ny;
        walkPhase += fabsf(v) * 6.0f;
    }

    if (shoot && now - lastShot >= SHOOT_MS)
    {
        shootGun(now);
    }
    if (use)
    {
        useDoor();
    }
    if (state == ST_PLAY)
    {
        updateEnemies(now, dt);
    }
}

//----------------------------------------------------------------------------
// drawing
//----------------------------------------------------------------------------

static uint32_t wallColor(char t)
{
    switch (t)
    {
        case '2': return 0x4A7A4A;      // green stone
        case '3': return 0x50648C;      // blue metal
        case 'D': return 0x9A9AA4;      // door
        default:  return 0x8A6A44;      // brown brick
    }
}

static void drawWalls(void)
{
    int x;
    for (x = 0; x < SCREEN_W; x++)
    {
        float camX = 2.0f * x / (float)SCREEN_W - 1.0f;
        float rdx = dirX + planeX * camX;
        float rdy = dirY + planeY * camX;
        int   mapX = (int)px, mapY = (int)py;
        float ddx = (rdx == 0.0f) ? 1e30f : fabsf(1.0f / rdx);
        float ddy = (rdy == 0.0f) ? 1e30f : fabsf(1.0f / rdy);
        int   stepX, stepY, side = 0, n;
        float sideX, sideY, perp, wallX, f;
        char  tile = '1';
        int   lineH, y0, y1;
        uint32_t base;

        if (rdx < 0) { stepX = -1; sideX = (px - mapX) * ddx; }
        else         { stepX = 1;  sideX = (mapX + 1.0f - px) * ddx; }
        if (rdy < 0) { stepY = -1; sideY = (py - mapY) * ddy; }
        else         { stepY = 1;  sideY = (mapY + 1.0f - py) * ddy; }

        // DDA: jump to the next grid line in x or y until a wall is hit
        for (n = 0; n < 64; n++)
        {
            if (sideX < sideY) { sideX += ddx; mapX += stepX; side = 0; }
            else               { sideY += ddy; mapY += stepY; side = 1; }
            if (mapX < 0 || mapY < 0 || mapX >= MAP_W || mapY >= MAP_H) { tile = '1'; break; }
            tile = map[mapY][mapX];
            if (tile != '.' && tile != 'O') break;
        }

        perp = (side == 0) ? sideX - ddx : sideY - ddy;   // no fish-eye
        if (perp < 0.05f) perp = 0.05f;
        zbuf[x] = perp;

        lineH = (int)(VIEW_H / perp);
        y0 = VIEW_H / 2 - lineH / 2;
        y1 = y0 + lineH - 1;

        wallX = (side == 0) ? py + perp * rdy : px + perp * rdx;
        wallX -= floorf(wallX);

        // light: darker with distance, y-sides darker (fake sunlight)
        f = 1.0f - perp / 12.0f;
        if (f < 0.18f) f = 0.18f;
        if (side == 1) f *= 0.7f;

        base = wallColor(tile);
        if (tile == 'D')
        {
            if (wallX > 0.45f && wallX < 0.55f) f *= 0.5f;     // door gap
            if (wallX < 0.08f || wallX > 0.92f) f *= 0.6f;     // frame
        }
        else if (wallX < 0.05f || (wallX > 0.48f && wallX < 0.52f))
        {
            f *= 0.6f;                                         // vertical joints
        }

        vline(x, y0, y1, shade(base, f));

        // horizontal mortar lines on brick walls
        if (tile == '1' && lineH > 12)
        {
            int k;
            for (k = 1; k < 4; k++)
            {
                int yy = y0 + lineH * k / 4;
                vline(x, yy, yy, shade(base, f * 0.55f));
            }
        }
    }
}

static void drawEnemy(const Enemy *e, uint32_t now)
{
    float tX, tY, f;
    int   screenX, h, xStart, yTop, floorY, stripe, r;
    const char *const *pic = (e->hp == 0) ? CORPSE : IMP;
    bool  white = now < e->hurtUntil;

    toCamera(e->x, e->y, &tX, &tY);
    if (tY < 0.2f) return;                       // behind the player

    screenX = (int)((SCREEN_W / 2) * (1.0f + tX / tY));
    h = (int)(VIEW_H / tY * 0.75f);
    floorY = VIEW_H / 2 + (int)(VIEW_H / tY / 2.0f);
    yTop = floorY - h;
    xStart = screenX - h / 2;
    f = 1.0f - tY / 12.0f;
    if (f < 0.25f) f = 0.25f;

    for (stripe = xStart; stripe < xStart + h; stripe++)
    {
        int tex;
        if (stripe < 0 || stripe >= SCREEN_W || tY >= zbuf[stripe]) continue;
        tex = (stripe - xStart) * 8 / h;
        for (r = 0; r < 8; r++)
        {
            char c = pic[r][tex];
            uint32_t col;
            if (c == '.') continue;
            if (white)          col = 0xFFFFFF;
            else if (c == '1')  col = shade(0xB83A22, f);
            else if (c == '2')  col = shade(0xFFE040, f + 0.3f);
            else if (c == '4')  col = shade(0xE8E8E8, f);
            else                col = shade(0x501414, f);
            vline(stripe, yTop + r * h / 8, yTop + (r + 1) * h / 8 - 1, col);
        }
    }
}

static void drawEnemies(uint32_t now)
{
    // painter's algorithm: far enemies first
    int   order[ENEMY_COUNT];
    float dist[ENEMY_COUNT];
    int   i, j;

    for (i = 0; i < ENEMY_COUNT; i++)
    {
        float dx = enemies[i].x - px, dy = enemies[i].y - py;
        order[i] = i;
        dist[i] = dx * dx + dy * dy;
    }
    for (i = 1; i < ENEMY_COUNT; i++)            // insertion sort, 5 elements
    {
        int k = order[i];
        j = i - 1;
        while (j >= 0 && dist[order[j]] < dist[k])
        {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = k;
    }
    for (i = 0; i < ENEMY_COUNT; i++)
    {
        drawEnemy(&enemies[order[i]], now);
    }
}

static void drawGun(uint32_t now)
{
    int ox = (int)(sinf(walkPhase) * 3.0f);
    int oy = (int)(fabsf(cosf(walkPhase)) * 2.0f);
    bool firing = now < flashUntil;

    if (firing)
    {
        oy += 3;                                         // recoil
        GrContextForegroundSet(&ctx, 0xFFC020);
        GrCircleFill(&ctx, 64 + ox, 84, 7);
        GrContextForegroundSet(&ctx, 0xFFFFE0);
        GrCircleFill(&ctx, 64 + ox, 84, 3);
    }
    fillRect(60 + ox, 90 + oy, 67 + ox, VIEW_H - 1, 0x5A5A60);   // barrel
    fillRect(61 + ox, 90 + oy, 61 + ox, VIEW_H - 1, 0x9A9AA0);   // highlight
    fillRect(53 + ox, 101 + oy, 74 + ox, VIEW_H - 1, 0x3A3A40);  // body
    fillRect(56 + ox, 104 + oy, 71 + ox, VIEW_H - 1, 0x6A4A2A);  // grip

    // crosshair
    GrContextForegroundSet(&ctx, 0x80FF80);
    GrLineDrawH(&ctx, 60, 62, VIEW_H / 2);
    GrLineDrawH(&ctx, 66, 68, VIEW_H / 2);
    GrLineDrawV(&ctx, 64, VIEW_H / 2 - 4, VIEW_H / 2 - 2);
    GrLineDrawV(&ctx, 64, VIEW_H / 2 + 2, VIEW_H / 2 + 4);
}

static void drawHud(void)
{
    char text[10] = "KILLS 0/0";
    uint32_t barColor = (health > 50) ? 0x30D030 : (health > 25) ? 0xE0C020 : 0xE03020;

    fillRect(0, HUD_Y, 127, 127, 0x2A2A2A);
    GrContextForegroundSet(&ctx, 0x707070);
    GrLineDrawH(&ctx, 0, 127, HUD_Y);

    GrContextForegroundSet(&ctx, ClrWhite);
    GrStringDraw(&ctx, "HP", -1, 3, 117, false);
    GrContextForegroundSet(&ctx, 0x808080);
    {
        tRectangle frame = { 16, 116, 58, 124 };
        GrRectDraw(&ctx, &frame);
    }
    if (health > 0)
    {
        fillRect(17, 117, 17 + health * 40 / 100, 123, barColor);
    }

    text[6] = '0' + kills;
    text[8] = '0' + ENEMY_COUNT;
    GrContextForegroundSet(&ctx, ClrWhite);
    GrStringDraw(&ctx, text, -1, 70, 117, false);
}

static void drawCentredBox(const char *line1, uint32_t c1, const char *line2)
{
    fillRect(8, 36, 119, 76, 0x101010);
    GrContextForegroundSet(&ctx, c1);
    {
        tRectangle frame = { 8, 36, 119, 76 };
        GrRectDraw(&ctx, &frame);
    }
    GrContextFontSet(&ctx, &g_sFontCmss20b);
    GrStringDrawCentered(&ctx, line1, -1, 64, 50, false);
    GrContextFontSet(&ctx, &g_sFontFixed6x8);
    GrContextForegroundSet(&ctx, ClrWhite);
    GrStringDrawCentered(&ctx, line2, -1, 64, 68, false);
}

static void drawTitle(void)
{
    fillRect(0, 0, 127, 127, ClrBlack);
    fillRect(0, 96, 127, 127, 0x3A1A10);
    GrContextFontSet(&ctx, &g_sFontCmss20b);
    GrContextForegroundSet(&ctx, 0x601010);
    GrStringDrawCentered(&ctx, "MINI DOOM", -1, 65, 21, false);   // shadow
    GrContextForegroundSet(&ctx, 0xE03020);
    GrStringDrawCentered(&ctx, "MINI DOOM", -1, 64, 20, false);
    GrContextFontSet(&ctx, &g_sFontFixed6x8);
    GrContextForegroundSet(&ctx, ClrWhite);
    GrStringDrawCentered(&ctx, "Stick: laufen/drehen", -1, 64, 48, false);
    GrStringDrawCentered(&ctx, "S1: schiessen", -1, 64, 60, false);
    GrStringDrawCentered(&ctx, "S2: Tuer oeffnen", -1, 64, 72, false);
    GrContextForegroundSet(&ctx, 0xFFC020);
    GrStringDrawCentered(&ctx, "S1 = Start", -1, 64, 110, false);
}

void game_init(void)
{
    Crystalfontz128x128_Init();
    GrContextInit(&ctx, &g_sCrystalfontz128x128);
    GrContextFontSet(&ctx, &g_sFontFixed6x8);
    state = ST_TITLE;
}

void game_draw(uint32_t now)
{
    if (state == ST_TITLE)
    {
        drawTitle();
        GrFlush(&ctx);
        return;
    }

    // ceiling and floor (floor in 4 bands: lighter towards the player)
    fillRect(0, 0, 127, VIEW_H / 2 - 1, 0x1C1C24);
    fillRect(0, VIEW_H / 2, 127, VIEW_H / 2 + 7, 0x241A12);
    fillRect(0, VIEW_H / 2 + 8, 127, VIEW_H / 2 + 19, 0x30241A);
    fillRect(0, VIEW_H / 2 + 20, 127, VIEW_H / 2 + 35, 0x3E2E20);
    fillRect(0, VIEW_H / 2 + 36, 127, VIEW_H - 1, 0x4C3826);

    drawWalls();
    drawEnemies(now);
    drawGun(now);
    drawHud();

    if (now < hurtUntil || state == ST_DEAD)
    {
        tRectangle r1 = { 0, 0, 127, VIEW_H - 1 };
        tRectangle r2 = { 1, 1, 126, VIEW_H - 2 };
        GrContextForegroundSet(&ctx, 0xE01010);
        GrRectDraw(&ctx, &r1);
        GrRectDraw(&ctx, &r2);
    }
    if (state == ST_DEAD)
    {
        drawCentredBox("GAME OVER", 0xE03020, "S2: Neustart");
    }
    else if (state == ST_WIN)
    {
        drawCentredBox("GESCHAFFT", 0x30D030, "S2: Neustart");
    }

    GrFlush(&ctx);
}
