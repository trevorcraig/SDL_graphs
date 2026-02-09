#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdlib.h>
#include <stdio.h>
#include "sdl_toolbar.h"
#include "sdl_graphs.h"
#include <math.h>

/**
 * @brief Allocates and initializes a new Toolbar and its associated SDL window.
 * * This function performs the following initialization steps:
 * 1. Allocates heap memory for the Toolbar structure.
 * 2. Sets up a secondary SDL window ("Graph Controls") and its renderer.
 * 3. Initializes UI components with hardcoded layout coordinates (FRects):
 * - **Line Navigator**: Prev/Next buttons for cycling through multiple plot lines.
 * - **Style Buttons**: Presets for Solid, Dashed, and Dotted lines.
 * - **Thickness Slider**: Sets the track and interactive handle positions.
 * - **Grid Toggle**: A checkbox for global grid visibility.
 * - **Color Swatches**: Preset color selection boxes.
 * - **Action Buttons**: The "Save as PNG" button layout.
 * * @param target Pointer to the Figure (main graph) that this toolbar will control.
 * @return Toolbar* A pointer to the newly created Toolbar instance, or NULL if allocation fails.
 * * @note The toolbar maintains a pointer to the Figure; ensure the Figure is not 
 * destroyed while the Toolbar is active to avoid dangling pointers.
 */
Toolbar* create_toolbar(Figure* target) {
    Toolbar* tb = malloc(sizeof(Toolbar));
    tb->target_fig = target;
    tb->running = true;
    tb->thickness_slider.track = (SDL_FRect){25, 270, 200, 10};
    tb->thickness_slider.handle = (SDL_FRect){25, 260, 15, 30};
    tb->thickness_slider.is_dragging = false;
    tb->thickness_slider.value = 0.0f;
    tb->active_axes_idx = 0;
    tb->active_line_idx = 0;

    // Create a small side-window
    tb->window = SDL_CreateWindow("Graph Controls", 250, 500, 0);
    tb->renderer = SDL_CreateRenderer(tb->window, NULL);

    tb->prev_line_btn.rect = (SDL_FRect){25, 10, 40, 30};
    tb->prev_line_btn.label = "<";

    tb->next_line_btn.rect = (SDL_FRect){185, 10, 40, 30};
    tb->next_line_btn.label = ">";

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

/**
 * @brief Renders the entire toolbar user interface.
 * * This function handles the drawing lifecycle for the toolbar window. It performs
 * the following steps:
 * 1. Clears the window with a light-gray background.
 * 2. Draws the Line Navigator (Previous/Next buttons and the current selection status).
 * 3. Renders Line Style buttons (Solid, Dashed, Dotted).
 * 4. Draws the Thickness Slider track and handle.
 * 5. Renders the Grid Toggle checkbox with visual "checked" state.
 * 6. Draws the Color Swatches with black borders.
 * 7. Renders the "Save as PNG" button.
 * 8. Presents the final frame to the display.
 * * @param tb Pointer to the Toolbar instance containing the UI state and renderer.
 * @param font The TTF_Font used for rendering all labels and button text.
 */
void render_toolbar(Toolbar* tb, TTF_Font* font) {
    SDL_SetRenderDrawColor(tb->renderer, 240, 240, 240, 255);
    SDL_RenderClear(tb->renderer);

    SDL_Color black = {0, 0, 0, 255};
    Axes* cur_ax = &tb->target_fig->axes[tb->active_axes_idx];

    // --- Render Line Navigator ---
    SDL_SetRenderDrawColor(tb->renderer, 200, 200, 200, 255);
    SDL_RenderFillRect(tb->renderer, &tb->prev_line_btn.rect);
    SDL_RenderFillRect(tb->renderer, &tb->next_line_btn.rect);
    draw_text(tb->renderer, font, tb->prev_line_btn.label, tb->prev_line_btn.rect.x + 15, tb->prev_line_btn.rect.y + 15, false, black);
    draw_text(tb->renderer, font, tb->next_line_btn.label, tb->next_line_btn.rect.x + 15, tb->next_line_btn.rect.y + 15, false, black);

    // Draw Status Label: "Line 1 / 3"
    char status_text[32];
    sprintf(status_text, "Line %d / %d", tb->active_line_idx + 1, cur_ax->line_count);
    draw_text(tb->renderer, font, status_text, 112, 35, false, black);

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
    if (cur_ax->show_grid){ // Note would like to test this with multigraph support
    // if (tb->target_fig->axes[0].show_grid) { // Leaving this for now to remind myself
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

/**
 * @brief Processes SDL events for the toolbar window.
 * * This function handles all user interactions within the toolbar, including:
 * - **Line Navigation**: Switching between multiple plot lines using arrows.
 * - **Styling**: Updating color and line style (solid, dashed, dotted) for the active line.
 * - **Global Settings**: Toggling the grid for all axes.
 * - **Interactivity**: Managing slider dragging for line thickness.
 * - **Actions**: Triggering the "Save as PNG" functionality.
 * * @param tb Pointer to the Toolbar instance.
 * @param event Pointer to the SDL_Event to be processed.
 * * @note This function filters events by window ID to ensure the toolbar only 
 * responds to interactions within its own window.
 */
void handle_toolbar_events(Toolbar* tb, SDL_Event* event) {
    // 1. BUTTON CLICKS (Style)
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (event->button.windowID != SDL_GetWindowID(tb->window)) return;
        
        float mx = event->button.x;
        float my = event->button.y;
        Axes* current_ax = &tb->target_fig->axes[tb->active_axes_idx];

        // Next Line
        if (mx >= tb->next_line_btn.rect.x && mx <= tb->next_line_btn.rect.x + tb->next_line_btn.rect.w &&
            my >= tb->next_line_btn.rect.y && my <= tb->next_line_btn.rect.y + tb->next_line_btn.rect.h) {
            tb->active_line_idx = (tb->active_line_idx + 1) % current_ax->line_count;
        }

        // Previous Line
        if (mx >= tb->prev_line_btn.rect.x && mx <= tb->prev_line_btn.rect.x + tb->prev_line_btn.rect.w &&
            my >= tb->prev_line_btn.rect.y && my <= tb->prev_line_btn.rect.y + tb->prev_line_btn.rect.h) {
            tb->active_line_idx--;
            if (tb->active_line_idx < 0) tb->active_line_idx = current_ax->line_count - 1;
        }

        // 1. Color Swatches
        for (int i = 0; i < 3; i++) {
            SDL_FRect* r = &tb->color_swatches[i].rect;
            if (mx >= r->x && mx <= r->x + r->w && my >= r->y && my <= r->y + r->h) {
                // tb->target_fig->axes[0].lines[0].color = tb->color_swatches[i].color;
                tb->target_fig->axes[tb->active_axes_idx].lines[tb->active_line_idx].color = tb->color_swatches[i].color;
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

        // Style buttons
        for (int i = 0; i < 3; i++) {
            SDL_FRect r = tb->buttons[i].rect;
            // Check if mouse is inside the button rectangle
            if (mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h) {
                if (current_ax->line_count > 0) {
                    current_ax->lines[tb->active_line_idx].style = tb->buttons[i].action_id;
                    printf("Changed Line %d style to: %s\n", 
                            tb->active_line_idx + 1, tb->buttons[i].label);
                }
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
        tb->target_fig->axes[tb->active_axes_idx].lines[tb->active_line_idx].thickness = new_thickness;
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        tb->thickness_slider.is_dragging = false;
    }
}

/**
 * @brief Safely destroys the toolbar and releases all associated SDL resources.
 * * This function cleans up the SDL_Renderer and SDL_Window associated with the 
 * toolbar inspector. It also frees the memory allocated for the Toolbar struct itself.
 * It is safe to call this even if the pointer is NULL.
 * * @param tb A pointer to the Toolbar instance to be destroyed.
 * * @note This should be called before the main SDL_Quit to ensure proper 
 * resource deallocation.
 */
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