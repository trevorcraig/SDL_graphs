#include <stdio.h>
#include "sdl_graphs.h"

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO) || TTF_Init() < 0) {
        return -1;
    }

    // 1. Emulate: fig, ax = plt.subplots()
    Figure* fig = subplots("Matplotlib C-Rewrite Test", 800, 600);
    SDL_SetRenderDrawBlendMode(fig->renderer, SDL_BLENDMODE_BLEND);
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
    plot(&fig->axes[0], x_data, y_data, n, blue);
    fig->axes[0].lines[0].thickness = 5.0f;
    fig->axes[0].lines[0].style = STYLE_DASHED;

    plot(&fig->axes[0], x_data2, y_data2, 5, red);
    fig->axes[0].lines[1].thickness = 2.0f;
    fig->axes[0].lines[1].style = STYLE_SOLID;
    // set_linestyle(&fig->axes[0], 0, STYLE_DASHED);
    scatter(&fig->axes[0], x_data, y_data, n, green, 8.0f);
    set_legend(&fig->axes[0], true); // Turn it on!

    // Setting Titles
    set_title(&fig->axes[0], "Temperature Over Time");
    set_xlabel(&fig->axes[0], "Time (seconds)");
    set_ylabel(&fig->axes[0], "Celsius");

    // 4. Main Loop (Simple plt.show() logic)
    bool running = true;
    SDL_Event event;

    while (running) {

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } 
            // else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            //     // SDL3 provides the new dimensions in the event
            //     int new_w = event.window.data1;
            //     int new_h = event.window.data2;
                
            //     // Update the axes pixel positions based on the new window size
            //     update_layout(fig, new_w, new_h);
            // }
        }
        int w, h;
        // SDL_GetWindowSize(fig->window, &w, &h);
        SDL_GetRenderOutputSize(fig->renderer, &w, &h);
        update_layout(fig, w, h);
        // Clear Screen
        SDL_SetRenderDrawColor(fig->renderer, 255, 255, 255, 255); // White background
        SDL_RenderClear(fig->renderer);

        // Draw our Objects
        render_axes(fig->renderer, fig->font, &fig->axes[0]);

        SDL_RenderPresent(fig->renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup (Simplified for example)
    SDL_DestroyRenderer(fig->renderer);
    SDL_DestroyWindow(fig->window);
    SDL_Quit();
    return 0;
}