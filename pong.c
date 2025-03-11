#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define WIN_W 800
#define WIN_H 600

#define ARENA_COLOR_BG DARKGREEN
#define ARENA_COLOR_FG RAYWHITE
#define BALL_COLOR WHITE
#define PAD_COLOR RAYWHITE

const int ball_radius = 10;
const int thickness = 20;
const int standoff = 40;

const float max_speed = 100;

const float upper = -WIN_H/2;
const float lower =  WIN_H/2;
const float left  = -WIN_W/2;
const float right =  WIN_W/2;

struct ball {
    float x;
    float y;
    float vx;
    float vy;
    int speed;
    int enable;
};

struct ball ball = {0, 0, 5, 0, 1, 1};

struct pad {
    float x;
    float y;
    float depth;
    float length;
    float ymotion;
};

struct pad pad0 = {left  + standoff + thickness/2, 0, thickness, 100, 0};
struct pad pad1 = {right - standoff - thickness/2, 0, thickness, 100, 0};

int score[2] = {0,0};
int serve_timer = 0;

struct generator {
    int life;
    int life_max;
    float frequency;
};

struct generator gens[8];

void setup_generators() {
    for(int g = 0; g < 8; g++){
        gens[g].life = 0;
        gens[g].life_max = 8000;
        gens[g].frequency = 220;
    }
}

float squared(float x){ return x*x; }

void audio_callback(void *buffer, unsigned int frames)
{
    short *out = (short *)buffer;

    for (unsigned int i = 0; i < frames; i++)
    {
        out[i] = 0;

        for(int g = 0; g < 8; g++) {
            if(gens[g].life == 0) continue;
            int step = gens[g].life_max - gens[g].life;
            float env = 1.0;
            if(step < 20) {
                env = squared((float)step / 20);
            }
            if(step > gens[g].life_max - 20) {
                env = squared((float)(gens[g].life_max - step) / 20);
            }
            float t = (float)step / 44100;
            out[i] += env * 6000 * sin(2 * PI * gens[g].frequency * t);
            gens[g].life--;
        }

    }
}


void world_to_screen(float x, float y, float *outx, float *outy) {
    if(outx) *outx = x + WIN_W/2;
    if(outy) *outy = y + WIN_H/2;
}

void screen_to_world(float x, float y, float *outx, float *outy) {
    if(outx) *outx = x - WIN_W/2;
    if(outy) *outy = y - WIN_H/2;
}

float randf() {
    return (float) rand() / RAND_MAX;
}

void draw_arena() {
    int thickness = 10;
    int th = thickness/2;

    int ax, bx, cx, dx, ex, fx;
    int ay, by, cy, dy, ey, fy;

    // A   B   C
    //
    // D   E   F

    ax = dx = th;
    bx = ex = WIN_W / 2;
    cx = fx = WIN_W - th;
    ay = by = cy = th;
    dy = ey = fy = WIN_H - th;

    Color color = ARENA_COLOR_FG;

    ClearBackground(ARENA_COLOR_BG);
    DrawRectangle(ax - th, ay - th, 2*th, WIN_H, color);
    DrawRectangle(bx - th, by - th, 2*th, WIN_H, color);
    DrawRectangle(cx - th, cy - th, 2*th, WIN_H, color);
    DrawRectangle(ax - th, ay - th, WIN_W, 2*th, color);
    DrawRectangle(dx - th, dy - th, WIN_W, 2*th, color);
    
}


void draw_paddles() {

    float x0, y0, x1, y1;

    world_to_screen(pad0.x - thickness/2, pad0.y - pad0.length/2, &x0, &y0);
    world_to_screen(pad1.x - thickness/2, pad1.y - pad1.length/2, &x1, &y1);

    DrawRectangle(x0, y0, thickness, pad0.length, PAD_COLOR);
    DrawRectangle(x1, y1, thickness, pad1.length, PAD_COLOR);
    
}

void draw_ball() {

    if(ball.enable == 0) return;

    float r = ball_radius;
    float x, y;

    world_to_screen(ball.x, ball.y, &x, &y);

    DrawRectangle(x - r, y - r, 2*r, 2*r, BALL_COLOR);

}

void draw_scores() {
    char buffer[16] = "0";
    int size = 80;
    float width = MeasureText(buffer, size);

    sprintf(buffer, "%d", score[0]);
    DrawText(buffer, 200-width/2, 50, size, RAYWHITE);
    sprintf(buffer, "%d", score[1]);
    DrawText(buffer, 600-width/2, 50, size, RAYWHITE);
}

void draw_winner() {
    int y = 150;
    if(score[0] == 9){
        DrawText("YOU", 100, y, 100, RAYWHITE);
        DrawText("WIN", 500, y, 100, RAYWHITE);
    }
    else if(score[1] == 9){
        DrawText("YOU", 100, y, 100, RAYWHITE);
        DrawText("LOSE", 480, y, 100, RAYWHITE);
    }
}

void play_sound(int sound_no) {
    for(int g = 0; g < 8; g++) {
        if(gens[g].life > 0) continue;
        switch(sound_no) {
            case 1:
                gens[g].life_max = 8000;
                gens[g].life = 8000;
                gens[g].frequency = 220;
                break;
            case 2:
                gens[g].life_max = 8000;
                gens[g].life = 8000;
                gens[g].frequency = 440;
                break;
            case 3:
                gens[g].life_max = 16000;
                gens[g].life = 16000;
                gens[g].frequency = 500 + 20*(score[0] > score[1] ? score[0] : score[1]);
                break;
        }
        return;
    }
}

Vector2 get_world_mouse() {
    Vector2 screen_mouse = GetMousePosition(); 
    Vector2 out;
    screen_to_world(screen_mouse.x, screen_mouse.y, &out.x, &out.y);
    return out;
}


int upper_hit_test(float y, float vy) {
    return vy < 0 && y - ball_radius < upper;
}

int lower_hit_test(float y, float vy) {
    return vy > 0 && y + ball_radius > lower;
}

int right_hit_test(float x, float y, float vx, float vy) {
    float r = ball_radius;
    Rectangle ball_rect = {x - r, y - r, 2*r, 2*r};

    float thick = thickness / 2;
    Rectangle pad_rect = {pad1.x - thick, pad1.y - pad1.length/2, thickness, pad1.length};

    return vx > 0 && CheckCollisionRecs(ball_rect, pad_rect);
}

int left_hit_test(float x, float y, float vx, float vy) {
    float r = ball_radius;
    Rectangle ball_rect = {x - r, y - r, 2*r, 2*r};

    float thick = thickness / 2;
    Rectangle pad_rect = {pad0.x - thick, pad0.y - pad0.length/2, thickness, pad0.length};

    return vx < 0 && CheckCollisionRecs(ball_rect, pad_rect);
}


float bump(float vy, float pad_vy) {
    float dvy = pad_vy/5.0;
    if(dvy >  10) dvy =  10;
    if(dvy < -10) dvy = -10;
    return vy + dvy;
}

/* move the ball over the course of a unit time interval */
/* the higher the speed, the more iterations are performed */
void ball_physics() {

    if(ball.enable == 0) return;

    float x  = ball.x;
    float y  = ball.y;
    float vx = ball.vx;
    float vy = ball.vy;

    float speed = sqrt(vx*vx + vy*vy);
    int N = floor(speed);
    if (N < 1) N = 1;

    for(int i = 0; i < N; i++) {
        int upper_hit = upper_hit_test(y, vy);
        int lower_hit = lower_hit_test(y, vy);
        int left_hit  = left_hit_test(x, y, vx, vy);
        int right_hit = right_hit_test(x, y, vx, vy);
        int left_goal = vx < 0 && x < left;
        int right_goal = vx > 0 && x > right;
        
        x += vx/speed;
        y += vy/speed;

        float boost = 1.0;
        if (speed < max_speed) boost = 1.1;

        if(upper_hit){ vy = -vy; play_sound(1); }
        if(lower_hit){ vy = -vy; play_sound(1); }
        if(left_hit){ vx = -vx*boost; vy = bump(vy, pad0.ymotion) * boost; play_sound(2); }
        if(right_hit){ vx = -vx*boost; vy *= boost; play_sound(2); }
        if(left_goal){ ball.enable = 0; score[1]++; serve_timer = 100; play_sound(3); break; }
        if(right_goal){ ball.enable = 0; score[0]++; serve_timer = 100; play_sound(3); break; }

        speed = sqrt(vx*vx + vy*vy);
    }


    ball.x = x;
    ball.y = y;
    ball.vx = vx;
    ball.vy = vy;

}

void serve_routine() {
    if(serve_timer == 0) return;
    if(score[0] == 9 || score[1] == 9) return;

    serve_timer -= 1;

    if(serve_timer == 0){

        ball.x = 0;
        ball.y = 0;
        ball.vx = 5;
        ball.vy = 10 * randf() - 10;
        ball.enable = 1;

    }
}

void player_control() {
    Vector2 motion = GetMouseDelta();
    pad0.y += motion.y;
    int bound = WIN_H/2 - pad0.length/2;
    if(pad0.y < -bound) pad0.y = -bound;
    if(pad0.y >  bound) pad0.y =  bound;

    pad0.ymotion = motion.y;
}

void computer_control() {
    /* sets computer players pad to Y coordinate of ball
    making the computer unbeatable */
    float delta = ball.y - pad1.y;
    pad1.y = ball.y;
    pad1.ymotion = delta;
}


int main(int argc, char * argv[]) {
    InitWindow(WIN_W, WIN_H, "PONG");

    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(4096);
    AudioStream stream = LoadAudioStream(44100, 16, 1);
    SetAudioStreamCallback(stream, audio_callback);
    PlayAudioStream(stream);

    //ToggleBorderlessWindowed();
    DisableCursor();

    SetTargetFPS(60);

    setup_generators();

    for(;;) {

        // CTRL
        player_control();
        computer_control();

        // FIZIKS
        ball_physics();
        serve_routine();

        // GRAPHICS
        BeginDrawing();

        draw_arena();
        draw_paddles();
        draw_scores();
        draw_ball();
        draw_winner();

        if(serve_timer > 0){
            //DrawText(TextFormat("%d", serve_timer), 100, 500, 20, WHITE);
        }

        EndDrawing();

        if(WindowShouldClose()) break;
    }

    UnloadAudioStream(stream);
    CloseAudioDevice();

    CloseWindow();

    return 0;

}
