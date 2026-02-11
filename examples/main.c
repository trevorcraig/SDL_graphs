#include <stdio.h>
#include "sdl_graphs.h"

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO) || TTF_Init() < 0) {
        return -1;
    }

    // 1. Emulate: fig, ax = plt.subplots()
    Figure* fig = subplots("Main Example", 800, 600,2);
    set_grid(&fig->axes[0], true); // Turn it on!
    
    
    // 2. Prepare Data: [1, 2, 3, 4], [1, 4, 2, 3]
    float x_data[] = {1.0f, 2.0f, 3.0f, 4.0f};
    float y_data[] = {1.0f, 4.0f, 2.0f, 3.0f};
    float x_data2[] = {1, 2, 3, 4, 5};
    float y_data2[] = {2, 4, 1, 5, 3};
    int n = 4;

    // 3. Emulate: ax.plot(x, y, color)
    SDL_Color blue = {50, 100, 255, 255};
    SDL_Color red = {255, 50, 50, 255};
    SDL_Color green = {50, 200, 50, 255};

    // Plot 1 (Left)
    plot(&fig->axes[0], x_data, y_data, n, blue);
    fig->axes[0].lines[0].thickness = 5.0f;
    fig->axes[0].lines[0].style = STYLE_DASHED;

    plot(&fig->axes[0], x_data2, y_data2, 5, red);
    fig->axes[0].lines[1].thickness = 2.0f;
    fig->axes[0].lines[1].style = STYLE_SOLID;
    scatter(&fig->axes[0], x_data, y_data, n, green, 8.0f);

    set_legend(&fig->axes[0], true); // Turn it on!

    // Plot 2 (Right)
    plot(&fig->axes[1], x_data, y_data, 4, red);
    set_grid(&fig->axes[1], true); // Turn it on!
    set_title(&fig->axes[1], "Pressure");
    fig->axes[1].lines[0].thickness = 2.0f;
    fig->axes[1].lines[0].style = STYLE_SOLID;
    
    // Setting Titles
    set_title(&fig->axes[0], "Temperature Over Time");
    set_xlabel(&fig->axes[0], "Time (seconds)");
    set_ylabel(&fig->axes[0], "Celsius");
    
    show(fig);
    TTF_Quit();
    SDL_Quit();
    return 0;
}