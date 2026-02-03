#include <stdio.h>
#include "sdl_graphs.h"
int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO) || TTF_Init() < 0) {
        return -1;
    }

    // 1. Create the figure with 2 subplots
    Figure* fig = subplots("Multi_Subplot Test", 800, 600, 2);
    if (!fig) return -1;

    SDL_SetRenderDrawBlendMode(fig->renderer, SDL_BLENDMODE_BLEND);
    
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

    // 4. Main Loop
    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } 
            // ONLY update layout when window changes
            if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                update_layout(fig, event.window.data1, event.window.data2);
            }
        }

        // Clear Screen
        SDL_SetRenderDrawColor(fig->renderer, 255, 255, 255, 255);
        SDL_RenderClear(fig->renderer);

        // Draw all Axes
        for(int i = 0; i < fig->axes_count; i++) {
            // Make sure fig->font is actually loaded in subplots()!
            render_axes(fig->renderer, fig->font, &fig->axes[i]);
        }

        SDL_RenderPresent(fig->renderer);
        SDL_Delay(16); 
    }

    // FINAL CLEANUP
    destroy_figure(fig); // This handles Window and Renderer too!
    TTF_Quit();
    SDL_Quit();
    return 0;
}