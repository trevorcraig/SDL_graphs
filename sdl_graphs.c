#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdlib.h>
#include <stdio.h>
#include "sdl_graphs.h"
#include <math.h>

// Emulates: fig, ax = plt.subplots()
Figure* subplots(const char* title, int width, int height) {
    Figure* fig = malloc(sizeof(Figure));
    fig->window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
    fig->renderer = SDL_CreateRenderer(fig->window, NULL);
    fig->font = TTF_OpenFont("PTC55F.ttf", 16);
    
    // Create one default Axes
    fig->axes = malloc(sizeof(Axes));
    fig->axes_count = 1;
    fig->axes[0].rect = (SDL_FRect){ 80, 60, width - 120, height - 120 };
    fig->axes[0].line_count = 0;
    fig->axes[0].lines = NULL;
    fig->axes[0].title = title;
    fig->axes[0].show_grid = false;
    fig->axes[0].show_legend = false;
    // Only for testing and may have to change later
    fig->axes[0].x_min = 1e38;  fig->axes[0].x_max = -1e38; 
    fig->axes[0].y_min = 1e38;  fig->axes[0].y_max = -1e38;
    
    return fig;
}

// Emulates: ax.plot(x, y)
void plot(Axes* ax, float* x, float* y, int count, SDL_Color color) {
    ax->lines = realloc(ax->lines, sizeof(Series) * (ax->line_count + 1));
    Series* newLine = &ax->lines[ax->line_count];

    newLine->x = x;
    newLine->y = y;
    newLine->count = count;
    newLine->color = color;
    newLine->type = PLOT_LINE; 
    newLine->marker_size = 0;
    newLine->thickness = 2.0f;
    ax->line_count++;
    strncpy(newLine->label, "Series", 32);

    // Update data limits (Auto-scaling)
    for(int i = 0; i < count; i++) {
        if (x[i] < ax->x_min) ax->x_min = x[i];
        if (x[i] > ax->x_max) ax->x_max = x[i];
        if (y[i] < ax->y_min) ax->y_min = y[i];
        if (y[i] > ax->y_max) ax->y_max = y[i];
    }
}

void scatter(Axes* ax, float* x, float* y, int count, SDL_Color color, float size) {
    // Reusing our existing plot logic but with a twist
    plot(ax, x, y, count, color); 
    
    // Set the last added series to scatter mode
    Series* s = &ax->lines[ax->line_count - 1];
    s->type = PLOT_SCATTER;
    s->marker_size = size;
}

void render_axes(SDL_Renderer* renderer, TTF_Font* font, Axes* ax) {
    const int tick_count = 5;
    const float tick_size = 5.0f;
    const SDL_Color text_color = {0, 0, 0, 255};
    
    float draw_w = ax->rect.w;
    float draw_h = ax->rect.h;
    float draw_x = ax->rect.x;
    float draw_y = ax->rect.y;

    // --- STEP 1: DRAW BACKGROUND FIRST ---
    SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
    SDL_RenderFillRect(renderer, &ax->rect);
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &ax->rect);

    // --- STEP 2: CALCULATE SCALING ---
    // Apply 10% padding so data doesn't touch the box edges
    float x_data_diff = ax->x_max - ax->x_min;
    float y_data_diff = ax->y_max - ax->y_min;
    float x_range = (x_data_diff > 0) ? x_data_diff * 1.1f : 1.0f;
    float y_range = (y_data_diff > 0) ? y_data_diff * 1.1f : 1.0f;

    // --- STEP 3: DRAW TICKS & LABELS ---
    for (int i = 0; i <= tick_count; i++) {
        float ratio = (float)i / tick_count;
        
        // Y Axis
        float y_pos = (draw_y + draw_h) - (ratio * draw_h);
        // float y_val = y_start_val + (ratio * y_range);
        float y_val = ax->y_min + (ratio * y_data_diff); 
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderLine(renderer, draw_x - tick_size, y_pos, draw_x, y_pos);
        
        char y_label[16];
        snprintf(y_label, sizeof(y_label), "%.1f", y_val);
        draw_text(renderer, font, y_label, draw_x - 10, y_pos, true, text_color);


        // X Axis
        float x_pos = draw_x + (ratio * draw_w);
        // float x_val = x_start_val + (ratio * x_range);
        float x_val = ax->x_min + (ratio * x_data_diff);
        SDL_RenderLine(renderer, x_pos, draw_y + draw_h, x_pos, draw_y + draw_h + tick_size);
        
        char x_label[16];
        snprintf(x_label, sizeof(x_label), "%.1f", x_val);
        draw_text(renderer, font, x_label, x_pos, draw_y + draw_h + 15, false, text_color);

        //Grid Lines
        if (ax->show_grid) {
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 100); // Light Gray
            SDL_RenderLine(renderer, draw_x, y_pos, draw_x + draw_w, y_pos);
            SDL_RenderLine(renderer, x_pos, draw_y, x_pos, draw_y + draw_h);
        }
    }
    // Render lines or scatter
    for (int l = 0; l < ax->line_count; l++) {
        Series* s = &ax->lines[l];
        SDL_SetRenderDrawColor(renderer, s->color.r, s->color.g, s->color.b, 255);
        
        for (int i = 0; i < s->count; i++) {
            // Map data to pixels
            float px = draw_x + ((s->x[i] - ax->x_min) / x_range) * draw_w;
            float py = (draw_y + draw_h) - ((s->y[i] - ax->y_min) / y_range) * draw_h;

            if (s->type == PLOT_LINE && i < s->count - 1) {
                // Existing line drawing logic...
                float px2 = draw_x + ((s->x[i+1] - ax->x_min) / x_range) * draw_w;
                float py2 = (draw_y + draw_h) - ((s->y[i+1] - ax->y_min) / y_range) * draw_h;

                if (s->style == STYLE_SOLID) {
                    RenderThickLine(renderer, px, py, px2, py2, s->thickness);
                } else {
                    DrawDashedThickLine(renderer, px, py, px2, py2, s->thickness, s->style);
                    // SDL_RenderLineDashed(renderer, px, py, px2, py2, s->style);
                }
                // // SDL_RenderLine(renderer, px, py, px2, py2);
                // SDL_RenderLineDashed(renderer, px, py, px2, py2, s->style);
            } 
            else if (s->type == PLOT_SCATTER) {
                // Draw a marker (square) centered on the point
                SDL_FRect marker = { 
                    px - (s->marker_size / 2.0f), 
                    py - (s->marker_size / 2.0f), 
                    s->marker_size, 
                    s->marker_size 
                };
                SDL_RenderFillRect(renderer, &marker);
            }
        }
    }
    render_legend(renderer, font, ax);
}

void draw_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, float x, float y, bool right_align, SDL_Color color) {
    if (!text || !font) return;

    SDL_Surface* surface = TTF_RenderText_Blended(font, text, 0, color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture) {
        float w = (float)surface->w;
        float h = (float)surface->h;
        
        // If right_align is true (for Y-axis), x is the right edge. 
        // Otherwise (for X-axis), x is the center.
        SDL_FRect dst = { 
            right_align ? x - w : x - (w / 2.0f), 
            y - (h / 2.0f), 
            w, h 
        };

        SDL_RenderTexture(renderer, texture, NULL, &dst);
        SDL_DestroyTexture(texture);
    }
    SDL_DestroySurface(surface);
}

void update_layout(Figure* fig, int window_w, int window_h) {
    for (int i = 0; i < fig->axes_count; i++) {
        Axes* ax = &fig->axes[i];
        
        // Example: 10% margins
        ax->rect.x = window_w * 0.10f;
        ax->rect.y = window_h * 0.10f;
        ax->rect.w = window_w * 0.80f;
        ax->rect.h = window_h * 0.75f; // Leave more room at bottom for labels
    }
}

void set_grid(Axes* ax, bool enabled) {
    ax->show_grid = enabled;
}

void SDL_RenderLineDashed(SDL_Renderer* renderer, float x1, float y1, float x2, float y2, LineStyle style) {
    if (style == STYLE_SOLID) {
        SDL_RenderLine(renderer, x1, y1, x2, y2);
        return;
    }

    float dx = x2 - x1;
    float dy = y2 - y1;
    float distance = sqrtf(dx * dx + dy * dy);
    float angle = atan2f(dy, dx);
    
    // Dash/Gap lengths in pixels
    float dashLen = (style == STYLE_DASHED) ? 10.0f : 2.0f;
    float gapLen = (style == STYLE_DASHED) ? 5.0f : 4.0f;
    float step = dashLen + gapLen;

    for (float i = 0; i < distance; i += step) {
        float segmentEnd = i + dashLen;
        if (segmentEnd > distance) segmentEnd = distance;

        float startX = x1 + cosf(angle) * i;
        float startY = y1 + sinf(angle) * i;
        float endX = x1 + cosf(angle) * segmentEnd;
        float endY = y1 + sinf(angle) * segmentEnd;

        SDL_RenderLine(renderer, startX, startY, endX, endY);
    }
}

void set_linestyle(Axes* ax, int series_idx, LineStyle style) {
    if (series_idx < ax->line_count) {
        ax->lines[series_idx].style = style;
    }
}

void RenderThickLine(SDL_Renderer* renderer, float x1, float y1, float x2, float y2, float thickness) {
    if (thickness <= 1.0f) {
        SDL_RenderLine(renderer, x1, y1, x2, y2);
        return;
    }

    // Calculate the direction vector
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    if (len == 0) return;

    // Calculate the offset vector (perpendicular to the line)
    float ux = dx / len;
    float uy = dy / len;
    float vx = -uy * (thickness / 2.0f);
    float vy = ux * (thickness / 2.0f);

    // Create 4 corners of the "thick line" rectangle
    SDL_Vertex vertices[4];
    SDL_Color c;
    SDL_GetRenderDrawColor(renderer, &c.r, &c.g, &c.b, &c.a);

    float coords[4][2] = {
        { x1 + vx, y1 + vy },
        { x1 - vx, y1 - vy },
        { x2 - vx, y2 - vy },
        { x2 + vx, y2 + vy }
    };

    for(int i=0; i<4; i++) {
        vertices[i].position.x = coords[i][0];
        vertices[i].position.y = coords[i][1];
        vertices[i].color.r = c.r / 255.0f; // SDL3 uses 0.0-1.0 for vertex colors
        vertices[i].color.g = c.g / 255.0f;
        vertices[i].color.b = c.b / 255.0f;
        vertices[i].color.a = c.a / 255.0f;
    }

    // Draw using two triangles (indices 0,1,2 and 0,2,3)
    int indices[6] = { 0, 1, 2, 0, 2, 3 };
    SDL_RenderGeometry(renderer, NULL, vertices, 4, indices, 6);
}

void DrawDashedThickLine(SDL_Renderer* renderer, float x1, float y1, float x2, float y2, float thickness, LineStyle style) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float distance = sqrtf(dx * dx + dy * dy);
    if (distance == 0) return;

    float angle = atan2f(dy, dx);
    
    // Define the dash and gap lengths
    float dashLen = (style == STYLE_DASHED) ? 10.0f : 3.0f;
    float gapLen = (style == STYLE_DASHED) ? 5.0f : 3.0f;
    float step = dashLen + gapLen;

    for (float i = 0; i < distance; i += step) {
        float segmentEnd = i + dashLen;
        if (segmentEnd > distance) segmentEnd = distance;

        // Calculate start and end of this specific dash
        float startX = x1 + cosf(angle) * i;
        float startY = y1 + sinf(angle) * i;
        float endX = x1 + cosf(angle) * segmentEnd;
        float endY = y1 + sinf(angle) * segmentEnd;

        // Draw this dash as a thick segment
        RenderThickLine(renderer, startX, startY, endX, endY, thickness);
    }
}

void render_legend(SDL_Renderer* renderer, TTF_Font* font, Axes* ax) {
    if (!ax->show_legend || ax->line_count == 0) return;

    // 1. Position the legend in the top-right corner of the axes
    float padding = 10.0f;
    float row_height = 20.0f;
    float box_w = 120.0f;
    float box_h = (ax->line_count * row_height) + (padding * 2);
    float box_x = (ax->rect.x + ax->rect.w) - box_w - padding;
    float box_y = ax->rect.y + padding;

    // 2. Draw Legend Background (Semi-transparent white)
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 180);
    SDL_FRect legend_rect = { box_x, box_y, box_w, box_h };
    SDL_RenderFillRect(renderer, &legend_rect);
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &legend_rect);

    // 3. Draw each entry
    for (int i = 0; i < ax->line_count; i++) {
        Series* s = &ax->lines[i];
        float entry_y = box_y + padding + (i * row_height) + (row_height / 2);

        // Draw the Style Proxy (A small line or dot)
        SDL_SetRenderDrawColor(renderer, s->color.r, s->color.g, s->color.b, 255);
        if (s->type == PLOT_LINE) {
            RenderThickLine(renderer, box_x + 5, entry_y, box_x + 25, entry_y, 2.0f);
        } else {
            SDL_FRect dot = { box_x + 12, entry_y - 3, 6, 6 };
            SDL_RenderFillRect(renderer, &dot);
        }

        // Draw the Label Text
        draw_text(renderer, font, s->label, box_x + 35, entry_y, false, (SDL_Color){0,0,0,255});
    }
}

void set_legend(Axes* ax, bool enabled) {
    ax->show_legend = enabled;
}

// And a way to name your lines:
void set_label(Axes* ax, int series_idx, const char* name) {
    if (series_idx < ax->line_count) {
        strncpy(ax->lines[series_idx].label, name, 31);
    }
}