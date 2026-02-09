#include "sdl_graphs.h"
#include "sdl_toolbar.h"

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO) || TTF_Init() < 0) {
        return -1;
    }

    // Create a 1x1 plot
    Figure* fig = subplots("ToolbarExanple", 800, 600, 1);
    Toolbar* tb = create_toolbar(fig);
    fig->toolbar = tb;
    
    float x[] = {1, 2, 3, 4, 5};
    float x2[] = {-1, 0, 3, 6, 9};
    float y[] = {10, 20, 15, 25, 30};
    float y2[] = {11, 22, 6, 18, 33};

    // Add data
    plot(&fig->axes[0], x, y, 5, (SDL_Color){50, 150, 255, 255});
    plot(&fig->axes[0], x2, y2, 5, (SDL_Color){50, 150, 255, 255});
    set_title(&fig->axes[0], "Growth Over Time");

    // Launch the window (blocks until closed)
    show(fig);

    TTF_Quit();
    SDL_Quit();
    return 0;
}