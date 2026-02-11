#include <stdio.h>
#include "sdl_graphs.h"
int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO) || TTF_Init() < 0) {
        return -1;
    }

    // 1. Create the figure with 2 subplots
    Figure* fig = subplots("Multi_Subplot Test", 800, 600, 2);
    if (!fig) return -1;
    
    // --- Data Setup ---
    float x_data[] = {1.0f, 2.0f, 3.0f, 4.0f};
    float y_data[] = {1.0f, 4.0f, 2.0f, 3.0f};
    
    // Plot to first axes
    plot(&fig->axes[0], x_data, y_data, 4, (SDL_Color){50, 100, 255, 255});
    set_title(&fig->axes[0], "Temperature Over Time");
    set_grid(&fig->axes[0], true);

    // Plot to second axes
    plot(&fig->axes[1], x_data, y_data, 4, (SDL_Color){255, 50, 50, 255});
    set_title(&fig->axes[1], "Pressure");
    set_grid(&fig->axes[1], true);
    
    show(fig);
    TTF_Quit();
    SDL_Quit();
    return 0;
}