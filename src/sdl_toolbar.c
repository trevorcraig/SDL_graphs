#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdlib.h>
#include <stdio.h>
#include "sdl_toolbar.h"
#include "sdl_graphs.h"
#include <math.h>

Toolbar* create_toolbar(Figure* target) {
    Toolbar* tb = malloc(sizeof(Toolbar));
    tb->target_fig = target;
    tb->running = true;
    tb->thickness_slider.track = (SDL_FRect){25, 270, 200, 10};
    tb->thickness_slider.handle = (SDL_FRect){25, 260, 15, 30};
    tb->thickness_slider.is_dragging = false;
    tb->thickness_slider.value = 0.0f;

    // Create a small side-window
    tb->window = SDL_CreateWindow("Graph Controls", 250, 500, 0);
    tb->renderer = SDL_CreateRenderer(tb->window, NULL);

    // Define 3 buttons for Line Styles
    const char* labels[] = {"Solid", "Dashed", "Dotted"};
    for (int i = 0; i < 3; i++) {
        tb->buttons[i].rect = (SDL_FRect){25, 50 + (i * 60), 200, 40};
        tb->buttons[i].color = (SDL_Color){100, 100, 100, 255};
        tb->buttons[i].label = labels[i];
        tb->buttons[i].action_id = i;
    }
    tb->grid_toggle.rect = (SDL_FRect){25, 330, 20, 20}; // A small square
    tb->grid_toggle.label = "Show Grid";
    SDL_Color colors[] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
    for (int i = 0; i < 3; i++) {
        tb->color_swatches[i].rect = (SDL_FRect){25 + (i * 70), 380, 50, 30};
        tb->color_swatches[i].color = colors[i];
        tb->color_swatches[i].label = ""; // No text needed for swatches
    }

    tb->save_button.rect = (SDL_FRect){25, 430, 200, 40};
    tb->save_button.label = "Save as PNG";

    return tb;
}

void render_toolbar(Toolbar* tb, TTF_Font* font) {
    SDL_SetRenderDrawColor(tb->renderer, 240, 240, 240, 255);
    SDL_RenderClear(tb->renderer);

    SDL_Color black = {0, 0, 0, 255};

    // 1. Render Buttons
    for (int i = 0; i < 3; i++) {
        SDL_FRect r = tb->buttons[i].rect;
        
        // Draw the button box
        SDL_SetRenderDrawColor(tb->renderer, 200, 200, 200, 255);
        SDL_RenderFillRect(tb->renderer, &r);
        
        // We pass the center of the rectangle: (x + w/2, y + h/2)
        draw_text(tb->renderer, font, tb->buttons[i].label, 
                  r.x + (r.w / 2.0f), r.y + (r.h / 2.0f), 
                  false, black);
    }

    // 2. Render Slider Label
    // Centered horizontally in the toolbar (approx x=125)
    draw_text(tb->renderer, font, "Line Thickness", 125, 240, false, black);

    // 3. Render Slider Track & Handle
    SDL_SetRenderDrawColor(tb->renderer, 180, 180, 180, 255);
    SDL_RenderFillRect(tb->renderer, &tb->thickness_slider.track);
    
    SDL_SetRenderDrawColor(tb->renderer, 50, 150, 255, 255);
    SDL_RenderFillRect(tb->renderer, &tb->thickness_slider.handle);

    SDL_Color check_blue = {50, 150, 255, 255};

    // Draw Checkbox Square
    SDL_SetRenderDrawColor(tb->renderer, 0, 0, 0, 255);
    SDL_RenderRect(tb->renderer, &tb->grid_toggle.rect);

    // If grid is enabled on the first axes, fill the box
    if (tb->target_fig->axes[0].show_grid) {
        SDL_SetRenderDrawColor(tb->renderer, check_blue.r, check_blue.g, check_blue.b, 255);
        // Shrink the inner fill slightly for a border effect
        SDL_FRect fill = { 
            tb->grid_toggle.rect.x + 3, 
            tb->grid_toggle.rect.y + 3, 
            tb->grid_toggle.rect.w - 6, 
            tb->grid_toggle.rect.h - 6 
        };
        SDL_RenderFillRect(tb->renderer, &fill);
    }

    // Draw label to the right of the checkbox
    draw_text(tb->renderer, font, tb->grid_toggle.label, 
            tb->grid_toggle.rect.x + 80, tb->grid_toggle.rect.y + 10, 
            false, black);

    // 4. Render Color Swatches
    for (int i = 0; i < 3; i++) {
        // Draw the colored box
        SDL_SetRenderDrawColor(tb->renderer, 
                               tb->color_swatches[i].color.r, 
                               tb->color_swatches[i].color.g, 
                               tb->color_swatches[i].color.b, 255);
        SDL_RenderFillRect(tb->renderer, &tb->color_swatches[i].rect);
        
        // Draw a black border around the swatch
        SDL_SetRenderDrawColor(tb->renderer, 0, 0, 0, 255);
        SDL_RenderRect(tb->renderer, &tb->color_swatches[i].rect);
    }

    // 5. Render Save Button
    SDL_SetRenderDrawColor(tb->renderer, 150, 150, 150, 255); // Darker gray for save
    SDL_RenderFillRect(tb->renderer, &tb->save_button.rect);
    SDL_SetRenderDrawColor(tb->renderer, 0, 0, 0, 255);
    SDL_RenderRect(tb->renderer, &tb->save_button.rect);

    // Use your draw_text for the button label
    draw_text(tb->renderer, font, tb->save_button.label, 
              tb->save_button.rect.x + (tb->save_button.rect.w / 2.0f), 
              tb->save_button.rect.y + (tb->save_button.rect.h / 2.0f), 
              false, black);
    SDL_RenderPresent(tb->renderer);
}

void handle_toolbar_events(Toolbar* tb, SDL_Event* event) {
    // 1. BUTTON CLICKS (Style)
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (event->button.windowID != SDL_GetWindowID(tb->window)) return;
        
        float mx = event->button.x;
        float my = event->button.y;

        // 1. Color Swatches
        for (int i = 0; i < 3; i++) {
            SDL_FRect* r = &tb->color_swatches[i].rect;
            if (mx >= r->x && mx <= r->x + r->w && my >= r->y && my <= r->y + r->h) {
                tb->target_fig->axes[0].lines[0].color = tb->color_swatches[i].color;
            }
        }

        // 2. Save Button
        SDL_FRect* sr = &tb->save_button.rect;
        if (mx >= sr->x && mx <= sr->x + sr->w && my >= sr->y && my <= sr->y + sr->h) {
            save_figure_as_png(tb->target_fig, "my_graph.png");
        }


        SDL_FRect gt = tb->grid_toggle.rect;

        if (mx >= gt.x && mx <= gt.x + gt.w && my >= gt.y && my <= gt.y + gt.h) {
            // Toggle grid for all axes in the figure
            for (int i = 0; i < tb->target_fig->axes_count; i++) {
                tb->target_fig->axes[i].show_grid = !tb->target_fig->axes[i].show_grid;
            }
        }

        // Check handle click for Slider
        SDL_FRect h = tb->thickness_slider.handle;
        if (mx >= h.x && mx <= h.x + h.w && my >= h.y && my <= h.y + h.h) {
            tb->thickness_slider.is_dragging = true;
        }

        for (int i = 0; i < 3; i++) {
            SDL_FRect r = tb->buttons[i].rect;
            // Check if mouse is inside the button rectangle
            if (mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h) {
                // Update ALL lines in the first axes as a test
                for(int j=0; j < tb->target_fig->axes[0].line_count; j++) {
                    tb->target_fig->axes[0].lines[j].style = tb->buttons[i].action_id;
                }
                printf("Changed style to: %s\n", tb->buttons[i].label);
            }
        }
    }

    // 2. SLIDER DRAGGING (Thickness)
    if (event->type == SDL_EVENT_MOUSE_MOTION && tb->thickness_slider.is_dragging) {
        float mx = event->motion.x;
        GraphSlider* s = &tb->thickness_slider;

        // Constraint within track
        if (mx < s->track.x) mx = s->track.x;
        if (mx > s->track.x + s->track.w) mx = s->track.x + s->track.w;

        // Update handle position
        s->handle.x = mx - (s->handle.w / 2.0f);
        
        // Calculate normalized value (0.0 to 1.0) and map to thickness (1.0 to 10.0)
        s->value = (mx - s->track.x) / s->track.w;
        float new_thickness = 1.0f + (s->value * 9.0f);

        // Apply to all lines in the first axes
        for(int j=0; j < tb->target_fig->axes[0].line_count; j++) {
            tb->target_fig->axes[0].lines[j].thickness = new_thickness;
        }
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        tb->thickness_slider.is_dragging = false;
    }
}

void destroy_toolbar(Toolbar* tb) {
    if (!tb) return;

    // 1. Destroy the SDL objects for the toolbar window
    if (tb->renderer) {
        SDL_DestroyRenderer(tb->renderer);
    }
    if (tb->window) {
        SDL_DestroyWindow(tb->window);
    }

    // 2. Free the struct memory itself
    free(tb);
    
    printf("Toolbar resources cleaned up.\n");
}