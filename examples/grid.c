#include "sdl_graphs.h"

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    // --- Data Setup ---
    float x_data[] = {1.0f, 2.0f, 3.0f, 4.0f};
    float y_data[] = {1.0f, 4.0f, 2.0f, 3.0f};
    int n=4;

    // Get me some colors
    SDL_Color blue = {50, 100, 255, 255};
    SDL_Color red = {255, 50, 50, 255};
    SDL_Color green = {50, 200, 50, 255};
    SDL_Color orange = {255, 200, 120, 255};

    // Create a 2x2 grid (4 axes)
    Figure* fig = subplots("Sensor Dashboard", 1024, 768, 4);

    // Plot 0: Top Left
    plot(&fig->axes[0], x_data, y_data, n, red);
    set_title(&fig->axes[0], "Temperature");

    // Plot 1: Top Right
    plot(&fig->axes[1], x_data, y_data, n, blue);
    set_title(&fig->axes[1], "Pressure");

    // Plot 2: Bottom Left
    plot(&fig->axes[2], x_data, y_data, n, green);
    set_title(&fig->axes[2], "Humidity");

    // Plot 3: Bottom Right
    plot(&fig->axes[3], x_data, y_data, n, orange);
    set_title(&fig->axes[3], "CO2 Levels");

    // Show the window
    show(fig);

    TTF_Quit();
    SDL_Quit();
    return 0;
}