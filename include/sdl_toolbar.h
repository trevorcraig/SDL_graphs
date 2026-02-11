#ifndef SDL_TOOLBAR_H
#define SDL_TOOLBAR_H

#include "sdl_graphs.h"


typedef struct {
    SDL_FRect rect;
    SDL_Color color;
    const char* label;
    int action_id; // 0: Solid, 1: Dashed, 2: Dotted
} GraphButton;

typedef struct {
    SDL_FRect track;    // The long bar
    SDL_FRect handle;   // The draggable knob
    float value;        // 0.0 to 1.0
    bool is_dragging;
} GraphSlider;

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    Figure* target_fig;
    GraphButton buttons[3];
    GraphButton color_swatches[3]; // Red, Green, Blue
    GraphSlider thickness_slider; // Our new slider
    GraphButton grid_toggle;
    GraphButton save_button;
    bool running;
    int active_axes_idx; 
    int active_line_idx;
    GraphButton next_line_btn;
    GraphButton prev_line_btn;
    GraphButton prev_ax_btn;
    GraphButton next_ax_btn;
} Toolbar;

// Initializes and opens the small control window
Toolbar* create_toolbar(Figure* target);

// The toolbar needs its own update/render loop
void update_toolbar(Toolbar* tb);

// Safely closes the toolbar
void destroy_toolbar(Toolbar* tb);

void render_toolbar(Toolbar* tb, TTF_Font* font);
void handle_toolbar_events(Toolbar* tb, SDL_Event* event);
static inline bool point_in_frect(float x, float y, SDL_FRect r);
#endif