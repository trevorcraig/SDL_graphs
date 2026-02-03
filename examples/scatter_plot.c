#include <stdio.h>
#include "sdl_graphs.h"

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO) || TTF_Init() < 0) {
        return -1;
    }

    // 1. Emulate: fig, ax = plt.subplots()
    Figure* fig = subplots("Simple Scatter Plot", 800, 600,1);
    SDL_SetRenderDrawBlendMode(fig->renderer, SDL_BLENDMODE_BLEND);
    set_grid(&fig->axes[0], true); // Turn it on!
    
    
    // 2. Prepare Data: [1, 2, 3, 4], [1, 4, 2, 3]
    float x_data[] = {1.0f, 2.0f, 3.0f, 4.0f};
    float y_data[] = {1.0f, 4.0f, 2.0f, 3.0f};
    int n = 4;

    // 3. Emulate: ax.plot(x, y, color)
    SDL_Color blue = {50, 100, 255, 255};
    scatter(&fig->axes[0], x_data, y_data, n, blue, 8.0f);
    set_legend(&fig->axes[0], true); // Turn it on!
    
    
    // Setting Titles
    set_title(&fig->axes[0], "Scatter Plot");
    set_xlabel(&fig->axes[0], "Time (seconds)");
    set_ylabel(&fig->axes[0], "Celsius");

    show(fig);
    TTF_Quit();
    SDL_Quit();
    return 0;
}