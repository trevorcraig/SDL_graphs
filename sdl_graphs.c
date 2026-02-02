#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdlib.h>
#include <stdio.h>
#include "sdl_graphs.h"
#include <math.h>

/**
 * @brief Initializes a new Figure with a default set of Axes.
 * * This function emulates the `plt.subplots()` behavior from Matplotlib. It creates
 * a resizable SDL window, initializes the hardware-accelerated renderer, loads 
 * the default font, and sets up a primary plotting area (Axes) with default margins.
 *
 * @param title  The text displayed in the window's title bar and as the default plot title.
 * @param width  The initial width of the window in pixels.
 * @param height The initial height of the window in pixels.
 * * @return A pointer to the newly allocated Figure object. 
 * @note   The returned Figure contains one Axes initialized with infinite min/max 
 * limits to allow the first call to plot() or scatter() to set the scale.
 */
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
    fig->axes[0].x_label = NULL;
    fig->axes[0].y_label = NULL;        
    fig->axes[0].show_grid = false;
    fig->axes[0].show_legend = false;
    // Only for testing and may have to change later
    fig->axes[0].x_min = 1e38;  fig->axes[0].x_max = -1e38; 
    fig->axes[0].y_min = 1e38;  fig->axes[0].y_max = -1e38;
    
    return fig;
}
/**
 * @brief Adds a line plot to the specified Axes.
 * * This function registers a new data series to be drawn as a continuous line. 
 * It automatically updates the axes' data limits (x_min, x_max, etc.) to 
 * ensure the new data is visible within the plot area.
 * * @param ax    Pointer to the Axes where the data should be plotted.
 * @param x     Array of x-coordinates (floats).
 * @param y     Array of y-coordinates (floats).
 * @param count The number of points in the x and y arrays.
 * @param color The SDL_Color to be used for the line.
 * * @note The library stores the pointers to the x and y arrays. Ensure the data 
 * remains valid in memory until the figure is destroyed.
 * @note Default line thickness is set to 2.0f and the default label is "Series".
 */
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

/**
 * @brief Adds a scatter plot to the specified Axes.
 * * This function utilizes the standard plot logic to register data and calculate 
 * auto-scaling limits, but sets the rendering type to discrete markers instead 
 * of connected lines.
 * * @param ax    Pointer to the Axes where the scatter points should be added.
 * @param x     Array of x-coordinates (floats).
 * @param y     Array of y-coordinates (floats).
 * @param count The number of points in the arrays.
 * @param color The SDL_Color to be used for the markers.
 * @param size  The diameter/side-length of the marker in pixels.
 * * @note Because this calls plot() internally, it also updates the axes' data limits.
 * @note Markers are currently rendered as filled squares centered on the data coordinates.
 */
void scatter(Axes* ax, float* x, float* y, int count, SDL_Color color, float size) {
    // Reusing our existing plot logic but with a twist
    plot(ax, x, y, count, color); 
    
    // Set the last added series to scatter mode
    Series* s = &ax->lines[ax->line_count - 1];
    s->type = PLOT_SCATTER;
    s->marker_size = size;
}


/**
 * @brief Performs the actual rendering of the axes, data, and metadata.
 * * This function should be called within the main render loop. It follows a specific
 * layering order to ensure visual clarity:
 * 1. Draws the plot background and border.
 * 2. Calculates dynamic scaling based on data limits and padding.
 * 3. Renders axis ticks, numerical labels, and the optional grid.
 * 4. Iterates through all Series to draw lines (solid/dashed) or scatter points.
 * 5. Overlays the legend and axis titles (Title, xlabel, ylabel).
 *
 * @param renderer The active SDL_Renderer.
 * @param font     The TTF_Font used for all text rendering in the plot.
 * @param ax       Pointer to the Axes object containing the data and configuration.
 * * @note This function handles coordinate transformation, mapping raw data values 
 * to screen pixels using the Axes' bounding box (rect).
 */
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
    SDL_Color black = {0, 0, 0, 255};

    // 1. Draw Main Title (Top Center)
    if (ax->title) {
        draw_text(renderer, font, ax->title, 
                ax->rect.x + (ax->rect.w / 2.0f), 
                ax->rect.y - 30, false, black);
    }

    // 2. Draw X-Axis Label (Bottom Center)
    if (ax->x_label) {
        draw_text(renderer, font, ax->x_label, 
                ax->rect.x + (ax->rect.w / 2.0f), 
                ax->rect.y + ax->rect.h + 40, false, black);
    }

    // 3. Draw Y-Axis Label (Left Center)
    if (ax->y_label) {
        // Note: Positioned to the left of the axis numbers
        draw_text(renderer, font, ax->y_label, 
                ax->rect.x - 60, 
                ax->rect.y + (ax->rect.h / 2.0f), false, black);
    }
}

/**
 * @brief Renders a string of text to the screen with specific alignment.
 * * This utility function automates the process of creating a surface from text, 
 * converting it to a texture, and rendering it. It includes logic to handle 
 * centering (for X-axis labels) or right-alignment (for Y-axis numbers).
 * * @param renderer    The active SDL_Renderer.
 * @param font        The pre-loaded TTF_Font to use.
 * @param text        The string to be displayed.
 * @param x           The horizontal anchor point.
 * @param y           The vertical anchor point (the text is always vertically centered on this).
 * @param right_align If true, 'x' is the right edge of the text. If false, 'x' is the horizontal center.
 * @param color       The SDL_Color for the text.
 * * @note This function performs texture creation and destruction on every call. 
 * For high-performance static text, consider caching textures.
 */
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

/**
 * @brief Recalculates the position and size of all Axes based on the window dimensions.
 * * This function implements a responsive layout by defining the plotting area using 
 * percentage-based margins. It ensures that axis labels, titles, and data points 
 * stay within the visible window area when the user resizes the window.
 * * @param fig      Pointer to the Figure whose layout needs updating.
 * @param window_w The current width of the window in pixels (usually from an SDL event).
 * @param window_h The current height of the window in pixels (usually from an SDL event).
 * * @note The current implementation uses a 15% margin on the top/left and 
 * reserves extra space at the bottom (35% total margin) to accommodate 
 * X-axis labels and titles.
 */
void update_layout(Figure* fig, int window_w, int window_h) {
    for (int i = 0; i < fig->axes_count; i++) {
        Axes* ax = &fig->axes[i];
        
        // Example: 15% margins
        ax->rect.x = window_w * 0.15f;
        ax->rect.y = window_h * 0.15f;
        ax->rect.w = window_w * 0.70f;
        ax->rect.h = window_h * 0.65f; // Leave more room at bottom for labels
    }
}

/**
 * @brief Renders a line between two points using a specific stroke style.
 * * Since SDL3 does not natively support non-solid line patterns, this function 
 * manually calculates the vector between (x1, y1) and (x2, y2) and iterates 
 * along the path, rendering discrete segments (dashes) or points (dots).
 * * @param renderer The active SDL_Renderer.
 * @param x1       The starting x-coordinate.
 * @param y1       The starting y-coordinate.
 * @param x2       The ending x-coordinate.
 * @param y2       The ending y-coordinate.
 * @param style    The LineStyle to apply (STYLE_SOLID, STYLE_DASHED, or STYLE_DOTTED).
 * * @note This function uses trigonometry (atan2f, cosf, sinf) to maintain the 
 * correct dash orientation regardless of the line's angle.
 * @note For dashed lines, segments are 10px with 5px gaps. For dotted lines, 
 * segments are 2px with 4px gaps.
 */
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

/**
 * @brief Renders a line with a specific pixel thickness using hardware geometry.
 * * Since standard SDL line rendering is limited to 1-pixel width, this function 
 * constructs a rotated rectangle around the line's path. It calculates the 
 * perpendicular "normal" vector to the line to offset the vertices by half the 
 * thickness in both directions.
 * * @param renderer  The active SDL_Renderer.
 * @param x1        Starting x-coordinate.
 * @param y1        Starting y-coordinate.
 * @param x2        Ending x-coordinate.
 * @param y2        Ending y-coordinate.
 * @param thickness The width of the line in pixels.
 * * @note This function uses SDL_RenderGeometry, which is much more efficient 
 * than drawing multiple parallel lines.
 * @note Vertex colors are normalized (0.0f - 1.0f) to match the SDL3 
 * SDL_Vertex specification.
 */
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

/**
 * @brief Renders a dashed or dotted line with a specific pixel thickness.
 * * This function iterates along the vector from (x1, y1) to (x2, y2), breaking 
 * the path into discrete segments based on the chosen LineStyle. Each individual 
 * dash is then rendered as a geometric primitive via RenderThickLine to support 
 * custom thickness.
 * * @param renderer  The active SDL_Renderer.
 * @param x1        Starting x-coordinate.
 * @param y1        Starting y-coordinate.
 * @param x2        Ending x-coordinate.
 * @param y2        Ending y-coordinate.
 * @param thickness The width of each dash in pixels.
 * @param style     The pattern to apply (STYLE_DASHED or STYLE_DOTTED).
 * * @note Dash/Gap lengths: 
 * - STYLE_DASHED: 10px dash, 5px gap.
 * - STYLE_DOTTED: 3px dash, 3px gap.
 * @note This function is more computationally expensive than RenderThickLine 
 * due to multiple geometry calls and trigonometric calculations per segment.
 */
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

// Setters

/**
 * @brief Toggles the visibility of the background grid lines.
 * * When enabled, the renderer will draw light-gray horizontal and vertical 
 * lines aligned with the axis ticks. This helps the viewer estimate 
 * data values more accurately across the plotting area.
 * * @param ax      Pointer to the Axes object to modify.
 * @param enabled Set to true to display the grid, false to hide it.
 * * @note Grid lines are rendered with partial transparency (alpha blending).
 * Ensure SDL_SetRenderDrawBlendMode is set to SDL_BLENDMODE_BLEND in your
 * main loop for the grid to appear correctly.
 */
void set_grid(Axes* ax, bool enabled) {
    ax->show_grid = enabled;
}

/**
 * @brief Sets the stroke pattern for a specific data series.
 * * Modifies how a series is rendered (solid, dashed, or dotted). This is 
 * particularly useful for distinguishing between multiple lines on the same 
 * axes or highlighting a specific trend.
 * * @param ax         Pointer to the Axes containing the series.
 * @param series_idx The index of the series (0 for the first plot() call, 1 for the second, etc.).
 * @param style      The LineStyle enum value (STYLE_SOLID, STYLE_DASHED, or STYLE_DOTTED).
 * * @note If the series_idx is out of bounds (greater than or equal to line_count), 
 * the function does nothing to prevent memory access violations.
 */
void set_linestyle(Axes* ax, int series_idx, LineStyle style) {
    if (series_idx < ax->line_count) {
        ax->lines[series_idx].style = style;
    }
}

/**
 * @brief Toggles the visibility of the plot legend.
 * * When enabled, a legend box is rendered (usually in the top-right corner) 
 * displaying the color, line style, and label for each data series added 
 * to the axes.
 * * @param ax      Pointer to the Axes object to modify.
 * @param enabled Set to true to display the legend, false to hide it.
 * * @note The legend labels default to "Series"; use set_label() to provide 
 * descriptive names for each plotted line or scatter set.
 */
void set_legend(Axes* ax, bool enabled) {
    ax->show_legend = enabled;
}

/**
 * @brief Assigns a descriptive name to a specific data series for the legend.
 * * This function updates the label string for a given series. This name is what 
 * will appear next to the color/style indicator in the legend box.
 * * @param ax         Pointer to the Axes containing the series.
 * @param series_idx The index of the series (based on the order they were added).
 * @param name       The string to display in the legend (max 31 characters).
 * * @note This function uses strncpy to safely copy the name into the internal 
 * buffer, ensuring the legend remains stable even if the original string 
 * is modified or goes out of scope.
 */
void set_label(Axes* ax, int series_idx, const char* name) {
    if (series_idx < ax->line_count) {
        strncpy(ax->lines[series_idx].label, name, 31);
    }
}

/**
 * @brief Sets the descriptive label for the horizontal (X) axis.
 * * This label is rendered centered below the plot area. It is typically 
 * used to describe the independent variable and its units (e.g., "Time (s)").
 * * @param ax    Pointer to the Axes object to modify.
 * @param label A pointer to a null-terminated string.
 * * @note The library stores the pointer directly. If you are using a 
 * dynamically generated string (e.g., from sprintf), ensure it remains 
 * valid as long as the plot is being rendered.
 */
void set_xlabel(Axes* ax, const char* label) {
    ax->x_label = label;
}

/**
 * @brief Sets the descriptive label for the vertical (Y) axis.
 * * This label is rendered to the left of the Y-axis tick marks. It is typically 
 * used to describe the dependent variable and its units (e.g., "Amplitude (mV)").
 * * @param ax    Pointer to the Axes object to modify.
 * @param label A pointer to a null-terminated string.
 * * @note The label position is automatically calculated by render_axes() to 
 * avoid overlapping with the numerical tick labels.
 * @warning The library stores the pointer directly. Ensure the string's memory 
 * remains valid for the duration of the rendering loop.
 */
void set_ylabel(Axes* ax, const char* label) {
    ax->y_label = label;
}

/**
 * @brief Sets the main descriptive title for the plotting area.
 * * The title is rendered at the top-center of the axes. It is the primary 
 * heading used to identify what the entire plot represents.
 * * @param ax    Pointer to the Axes object to modify.
 * @param title A pointer to a null-terminated string (e.g., "Real-time Signal Analysis").
 * * @note This function updates the pointer in the Axes struct. If you are 
 * updating the title dynamically in a loop, ensure the source string 
 * buffer is not destroyed before the next render call.
 */
void set_title(Axes* ax, const char* title) {
    ax->title = title;
}