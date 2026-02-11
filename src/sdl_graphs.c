#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#include <stdlib.h>
#include <stdio.h>
#include "sdl_graphs.h"
#include "sdl_toolbar.h"
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
 * @param num_axes The number of axes we are going to plot.
 * * @return A pointer to the newly allocated Figure object. 
 * @note   The returned Figure contains one Axes initialized with infinite min/max 
 * limits to allow the first call to plot() or scatter() to set the scale.
 */
Figure* subplots(const char* title, int width, int height, int num_axes) {
    Figure* fig = malloc(sizeof(Figure));
    if (!fig) return NULL;
    fig->window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
    // Left this for future look back in case nothing connects
    // printf("Available renderer drivers:\n");
    // for (int i = 0; i < SDL_GetNumRenderDrivers(); i++) {
    //     printf("%d. %s\n", i + 1, SDL_GetRenderDriver(i));
    // }
    // fig->renderer = SDL_CreateRenderer(fig->window, "direct3d12"); //was NULL orginally but direct3d11 which has an issue
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d12"); //direct3d11 has an issue that causes ocassional crashes
    fig->renderer = SDL_CreateRenderer(fig->window, NULL); //opengl is the best option
    SDL_SetRenderDrawBlendMode(fig->renderer, SDL_BLENDMODE_BLEND); // Ensure it to be true
    fig->font = TTF_OpenFont("PTC55F.ttf", 16);
    fig->toolbar = NULL;
    fig->axes_count = num_axes;
    fig->axes = malloc(sizeof(Axes) * num_axes);
    
    
    for(int i = 0; i < num_axes; i++) {
        fig->axes[i].line_count = 0;
        fig->axes[i].lines = NULL;
        fig->axes[i].title = title; // Note: All subplots start with the Figure title
        fig->axes[i].x_label = NULL;
        fig->axes[i].y_label = NULL;  
        fig->axes[i].z_label = NULL;        
        fig->axes[i].show_grid = false;
        fig->axes[i].show_legend = false;
        fig->axes[i].projection = PROJECTION_2D;
        
        // Corrected indexing here:
        fig->axes[i].x_min = 1e38f;  fig->axes[i].x_max = -1e38f; 
        fig->axes[i].y_min = 1e38f;  fig->axes[i].y_max = -1e38f;
        fig->axes[i].phi = 0.0f;
        fig->axes[i].theta = 0.0f;
        fig->axes[i].zoom = 1.0f;
        fig->axes[i].z_min = 1e38f;  fig->axes[i].z_max = -1e38f;
    }

    update_layout(fig, width, height); 
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
    newLine->style=STYLE_SOLID;
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
 * @brief Adds a 3D line plot to the axes.
 * Matches Matplotlib's ax.plot3D(x, y, z)
 */
void plot3D(Axes* ax, float* x, float* y, float* z, int count, SDL_Color color) {
    if (!ax || !x || !y || !z || count <= 0) return;

    // Ensure projection is 3D
    if (ax->projection != PROJECTION_3D) {
        set_projection(ax, PROJECTION_3D);
    }

    // Create new series
    ax->lines = realloc(ax->lines, sizeof(Series) * (ax->line_count + 1));
    Series* s = &ax->lines[ax->line_count];
    s->x = x;
    s->y = y;
    s->z = z; 
    s->count = count;
    s->color = color;
    s->type = PLOT_LINE;
    s->thickness = 2.0f;
    s->style = STYLE_SOLID;
    ax->line_count++;
    strncpy(s->label, "Series", 32);
    // --- Auto-scale bounds ---
    for (int i = 0; i < count; i++) {

        if (x[i] < ax->x_min) ax->x_min = x[i];
        if (x[i] > ax->x_max) ax->x_max = x[i];

        if (y[i] < ax->y_min) ax->y_min = y[i];
        if (y[i] > ax->y_max) ax->y_max = y[i];

        if (z[i] < ax->z_min) ax->z_min = z[i];
        if (z[i] > ax->z_max) ax->z_max = z[i];
    }
    // Safety: prevent zero ranges (avoids divide-by-zero in projection)
    if (ax->x_max == ax->x_min) ax->x_max += 1.0f;
    if (ax->y_max == ax->y_min) ax->y_max += 1.0f;
    if (ax->z_max == ax->z_min) ax->z_max += 1.0f;
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
 * @brief Performs 2D rendering of axes, grid lines, data series, and labels.
 * * This function follows a strict layering order to ensure visual clarity:
 * 1. **Background**: Draws the plot area and border.
 * 2. **Scaling**: Calculates normalization factors with 10% padding for data breathing room.
 * 3. **Axes & Grid**: Renders numerical ticks and optional light-gray grid lines.
 * 4. **Data Plotting**: Iterates through series to render either thick lines (solid/dashed) 
 * or scatter plot markers.
 * 5. **Annotations**: Overlays the legend and titles (Main, X, and Y).
 * * @param renderer The active SDL_Renderer.
 * @param font     The TTF_Font used for all text (labels, ticks, titles).
 * @param ax       Pointer to the Axes object containing 2D data and configuration.
 * * @note Coordinate mapping: Screen Y is inverted relative to data Y because SDL's 
 * origin (0,0) is at the top-left.
 */
void render_axes_2d(SDL_Renderer* renderer, TTF_Font* font, Axes* ax) {
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
 * @brief Renders 3D data series within a projected wireframe "cage".
 * * Unlike the 2D renderer, this function utilizes a projection engine to map 
 * (x, y, z) coordinates into 2D screen space based on the camera's current 
 * rotation (phi and theta).
 * * **Process:**
 * 1. **Cage**: Renders a 3D bounding box to provide a spatial reference.
 * 2. **Projection**: Iterates through series, projecting each 3D point to a screen 
 * pixel using `project_3d`.
 * 3. **Line Strips**: Connects projected points to form 3D paths.
 * 4. **Z-Labeling**: Places the Z-axis title at a projected anchor point relative to the cage.
 * * @param renderer The active SDL_Renderer.
 * @param font     The TTF_Font used for text rendering.
 * @param ax       Pointer to the Axes object (must have PROJECTION_3D enabled).
 * * @see project_3d, draw_3d_box
 */
void render_axes_3d(SDL_Renderer* renderer, TTF_Font* font, Axes* ax) {
    // 1. Draw the Bounding Box (The "Cage")
    draw_3d_box(renderer, ax);

    // 2. Render each 3D Series
    for (int i = 0; i < ax->line_count; i++) {
        Series* s = &ax->lines[i];
        if (s->count < 2) continue;

        // Set the series color
        SDL_SetRenderDrawColor(renderer, s->color.r, s->color.g, s->color.b, s->color.a);

        for (int j = 0; j < s->count - 1; j++) {
            float x1, y1, x2, y2;

            // Project the current point and the next point
            project_3d(ax, s->x[j], s->y[j], s->z[j], &x1, &y1);
            project_3d(ax, s->x[j+1], s->y[j+1], s->z[j+1], &x2, &y2);
            SDL_RenderLine(renderer, x1, y1, x2, y2);
        }
    }

    float lx, ly;
    SDL_Color label_color = {0, 0, 0, 255};
    // X label
    project_3d(ax, (ax->x_min + ax->x_max) * 0.5f, ax->y_min, ax->z_min, &lx, &ly);
    draw_text(renderer, font, ax->x_label, lx, ly + 30, false, label_color);

    // Y label
    project_3d(ax, ax->x_max, (ax->y_min + ax->y_max) * 0.5f, ax->z_min, &lx, &ly);
    draw_text(renderer, font, ax->y_label, lx + 35, ly + 15, false, label_color);

    // Z label
    project_3d(ax, ax->x_min, ax->y_min, (ax->z_min + ax->z_max) * 0.5f, &lx, &ly);
    draw_text(renderer, font, ax->z_label, lx - 40, ly, false, label_color);

}

/**
 * @brief High-level dispatcher that renders an Axes object based on its projection type.
 * * This acts as the primary interface for the Figure rendering loop. It inspects 
 * the Axes' `projection` member and routes the rendering task to either the 
 * 2D or 3D specific implementation.
 * * @param renderer The active SDL_Renderer.
 * @param font     The TTF_Font used for rendering.
 * @param ax       Pointer to the Axes object to be drawn.
 * * @note This abstraction allows a single Figure to contain a mix of 2D and 3D subplots.
 */
void render_axes(SDL_Renderer* renderer, TTF_Font* font, Axes* ax) {
    if (ax->projection == PROJECTION_3D) {
        render_axes_3d(renderer, font, ax);
    } else {
        render_axes_2d(renderer, font, ax);
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
    if (fig->axes_count <= 0) return;

    // 1. Determine Grid Dimensions
    int cols = (fig->axes_count > 1) ? 2 : 1; 
    int rows = (fig->axes_count + cols - 1) / cols;

    // 2. Calculate individual cell size
    float cell_w = (float)window_w / cols;
    float cell_h = (float)window_h / rows;

    for (int i = 0; i < fig->axes_count; i++) {
        Axes* ax = &fig->axes[i];

        int r = i / cols;
        int c = i % cols;

        // 3. Dynamic Padding
        // We need space for Y-axis labels (left) and X-axis labels (bottom)
        float pad_left   = cell_w * 0.12f; // Room for Y-axis numbers
        float pad_right  = cell_w * 0.05f; // Small gap on right
        float pad_top    = cell_h * 0.10f; // Room for Title
        float pad_bottom = cell_h * 0.15f; // Room for X-axis labels

        // 4. Set the Rectangle
        ax->rect.x = (c * cell_w) + pad_left;
        ax->rect.y = (r * cell_h) + pad_top;
        ax->rect.w = cell_w - (pad_left + pad_right);
        ax->rect.h = cell_h - (pad_top + pad_bottom);
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
    // SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND); // Ensured now in subplots
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
/**
 * @brief Safely deallocates a Figure and all its associated Axes and Series.
 * * This function performs a deep-clean of the Figure's memory. It frees the 
 * internal arrays for axes and lines, and destroys the SDL_Window and 
 * SDL_Renderer associated with the figure.
 * * @param fig Pointer to the Figure to be destroyed.
 * @note This does NOT free the raw data arrays (x, y) passed to plot(), 
 * as those are owned by the caller.
 */
void destroy_figure(Figure* fig) {
    if (!fig) return;

    // 1. Loop through each Axes
    for (int i = 0; i < fig->axes_count; i++) {
        Axes* ax = &fig->axes[i];

        // 2. Loop through each Series in the Axes
        for (int j = 0; j < ax->line_count; j++) {
            // If you later decide to malloc the x/y arrays inside the lib, 
            // you'd free them here. Currently, they are user-owned pointers.
        }
        
        // Free the array of Series
        if (ax->lines) {
            free(ax->lines);
        }
    }

    // 3. Free the array of Axes
    if (fig->axes) {
        free(fig->axes);
    }

    // 4. Clean up SDL Resources
    if (fig->renderer) {
        SDL_DestroyRenderer(fig->renderer);
    }
    if (fig->window) {
        SDL_DestroyWindow(fig->window);
    }

    // 5. Free the Figure itself
    free(fig);
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

/**
 * @brief Enters a blocking main loop to display the figure.
 * * This is a high-level convenience function similar to Matplotlib's plt.show().
 * It handles:
 * - The SDL event loop (Quit and Window Resize events).
 * - Automatic layout updates on resize.
 * - Background clearing and rendering of all subplots.
 * - Automatic memory cleanup via destroy_figure() upon closing.
 * * @param fig Pointer to the Figure to be displayed.
 * * @note This function is BLOCKING. It will not return until the user closes 
 * the window. For real-time data updates, do not use this function; 
 * instead, implement your own loop and call render_axes() manually.
 * * @warning Because this function calls destroy_figure() internally, the 'fig' 
 * pointer will be invalid after this function returns.
 */
void show(Figure* fig) {
    if (!fig) return;

    bool running = true;
    SDL_Event event;
    Toolbar* tb = (Toolbar*)fig->toolbar; // Cast the pointer
    

    while (running) {
        while (SDL_PollEvent(&event)) {
            // Handle Global Quit
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            // For multiwindow support
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                // 1. Check main window (Safe, because fig always exists)
                if (event.window.windowID == SDL_GetWindowID(fig->window)) {
                    running = false;
                }
                // 2. ONLY check toolbar if tb is NOT NULL
                else if (tb != NULL && tb->window != NULL) {
                    if (event.window.windowID == SDL_GetWindowID(tb->window)) {
                        running = false; 
                    }
                }
            }
            if (tb != NULL) {
                handle_toolbar_events(tb, &event);
            }

            // Route events to Graph Window (Resizing)
            if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                if (event.window.windowID == SDL_GetWindowID(fig->window)) {
                    update_layout(fig, event.window.data1, event.window.data2);
                }
            }

            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                // Check if left mouse button is held down
                if (event.motion.state & SDL_BUTTON_LMASK) {
                    // We only want to rotate the axes the mouse is actually over
                    for (int i = 0; i < fig->axes_count; i++) {
                        Axes* ax = &fig->axes[i];
                        if (ax->projection == PROJECTION_3D) {
                            // Sensitivity: 0.5 degrees per pixel moved
                            ax->phi   += event.motion.xrel * 0.5f; 
                            ax->theta -= event.motion.yrel * 0.5f;
                            
                            // Keep theta within reasonable bounds so the graph doesn't flip
                            if (ax->theta > 89.0f) ax->theta = 89.0f;
                            if (ax->theta < -89.0f) ax->theta = -89.0f;
                        }
                    }
                }
            }
        }

        // --- RENDER GRAPH WINDOW ---
        SDL_SetRenderDrawColor(fig->renderer, 255, 255, 255, 255);
        SDL_RenderClear(fig->renderer);
        for (int i = 0; i < fig->axes_count; i++) {
            render_axes(fig->renderer, fig->font, &fig->axes[i]);
        }
        SDL_RenderPresent(fig->renderer);

        // --- RENDER TOOLBAR WINDOW ---
        if (tb != NULL) {
            render_toolbar(tb, fig->font);
        }

        SDL_Delay(16);
    }

    // Clean up
    if (tb != NULL) destroy_toolbar(tb); //Destroy figure already has destroy_toolbar
    destroy_figure(fig);
}

/**
 * @brief Captures the current state of the graph renderer and saves it as a PNG file.
 * * This function performs a screen capture of the main figure window by:
 * 1. Reading the pixel data directly from the GPU renderer's current viewport.
 * 2. Converting that data into an intermediate SDL_Surface.
 * 3. Utilizing SDL_image's PNG export functionality to write the file to disk.
 * * @param fig Pointer to the Figure instance whose content should be saved.
 * @param filename The destination path and name of the file (e.g., "output.png").
 * * @note This operation is performed on the current back buffer. For best results, 
 * ensure the figure has been fully rendered before calling this function.
 * @warning This function requires the SDL_image library to be initialized.
 */
void save_figure_as_png(Figure* fig, const char* filename) {
    // 1. Grab the pixels from the renderer into a surface
    // NULL reads the entire viewport
    SDL_Surface* surface = SDL_RenderReadPixels(fig->renderer, NULL);
    
    if (surface) {
        // 2. Save using SDL_image's PNG function
        if (IMG_SavePNG(surface, filename)) {
            printf("Graph saved successfully to %s\n", filename);
        } else {
            fprintf(stderr, "Failed to save PNG: %s\n", SDL_GetError());
        }
        SDL_DestroySurface(surface);
    } else {
        fprintf(stderr, "Failed to read pixels: %s\n", SDL_GetError());
    }
}

/**
 * @brief Configures the projection type for a specific set of axes.
 * * If the projection is set to PROJECTION_3D, this function initializes the 
 * camera orientation (phi/theta) to standard Matplotlib-style defaults, 
 * resets the zoom level, and establishes baseline Z-axis boundaries and labels.
 * * @param[in,out] ax   Pointer to the Axes structure to modify.
 * @param[in]     proj The desired projection type (PROJECTION_2D or PROJECTION_3D).
 * * @note Default 3D view angles are set to an Azimuth (phi) of 300° and 
 * an Elevation (theta) of 30° to provide a clear isometric-like perspective.
 */
void set_projection(Axes* ax, ProjectionType proj) {
    if (!ax) return;
    
    ax->projection = proj;
    
    if (proj == PROJECTION_3D) {
        ax->phi = 300.0f;
        ax->theta = 30.0f; 
        ax->zoom = 1.0f;
        
        // Initialize Z bounds to something sensible
        ax->z_min = -1.0f;
        ax->z_max = 1.0f;
        ax->x_label= "X-Axis";
        ax->y_label= "Y-Axis";
        ax->z_label= "Z-Axis";
    }
}


/**
 * @brief Projects 3D data coordinates into 2D screen space using orthographic projection.
 * * This transformation follows a four-step pipeline:
 * 1. **Normalization**: Maps raw data (x, y, z) to a canonical cube range of [-1.0, 1.0].
 * 2. **Rotation (Azimuth)**: Rotates the point around the Z-axis by the angle 'phi'.
 * 3. **Rotation (Elevation)**: Rotates the resulting point around the X-axis by the angle 'theta'.
 * 4. **Screen Mapping**: Scales the rotated 3D point and offsets it to the center of the 
 * Axes' viewport, flipping the Y-axis to match SDL's coordinate system.
 * * @param[in]  ax Pointer to the Axes containing the 3D bounds and camera state.
 * @param[in]  x  The raw X data coordinate.
 * @param[in]  y  The raw Y data coordinate.
 * @param[in]  z  The raw Z data coordinate.
 * @param[out] px Pointer to store the resulting screen X-coordinate.
 * @param[out] py Pointer to store the resulting screen Y-coordinate.
 * * @note This function uses an orthographic projection, meaning parallel lines in 3D 
 * space remain parallel on screen (no perspective foreshortening).
 */
static void project_3d(Axes* ax, float x, float y, float z, float* px, float* py) {
    // 1. Normalize coordinates to a -1.0 to 1.0 range based on ax bounds
    float nx = 2.0f * (x - ax->x_min) / (ax->x_max - ax->x_min) - 1.0f;
    float ny = 2.0f * (y - ax->y_min) / (ax->y_max - ax->y_min) - 1.0f;
    float nz = 2.0f * (z - ax->z_min) / (ax->z_max - ax->z_min) - 1.0f;

    // 2. Convert angles to radians
    float rad_phi = ax->phi * (M_PI / 180.0f);
    float rad_theta = ax->theta * (M_PI / 180.0f);

    // 3. Apply Rotation (Azimuth/Elevation)
    // Rotate around Z (Azimuth)
    float x1 = nx * cosf(rad_phi) - ny * sinf(rad_phi);
    float y1 = nx * sinf(rad_phi) + ny * cosf(rad_phi);
    
    // Rotate around X (Elevation)
    float x2 = x1;
    float y2 = y1 * cosf(rad_theta) - nz * sinf(rad_theta);
    float z2 = y1 * sinf(rad_theta) + nz * cosf(rad_theta);

    // 4. Project to 2D screen space
    // Center of the axes area
    float cx = ax->rect.x + ax->rect.w / 2.0f;
    float cy = ax->rect.y + ax->rect.h / 2.0f;
    
    // Scale the projected points to fit the screen area
    float scale = (ax->rect.w < ax->rect.h ? ax->rect.w : ax->rect.h) * 0.4f * ax->zoom;

    *px = cx + x2 * scale;
    *py = cy - y2 * scale; // Subtract because SDL Y-axis goes down
}

/**
 * @brief Renders a wireframe bounding box (cage) for a 3D plot.
 * * This function calculates the 8 corners of the 3D data space defined by the 
 * Axes' min/max bounds, projects them into 2D screen space, and draws the 
 * connecting edges to form a cube. This "cage" provides necessary depth 
 * cues for the user to interpret the 3D data.
 * * @param renderer The SDL_Renderer used for drawing lines.
 * @param ax       Pointer to the Axes struct containing 3D bounds and 
 * camera rotation state (phi/theta).
 * * @note This function connects:
 * - 4 edges for the bottom face (z_min)
 * - 4 edges for the top face (z_max)
 * - 4 vertical edges connecting top and bottom faces.
 */
void draw_3d_box(SDL_Renderer* renderer, Axes* ax) {
    float corners[8][3] = {
        {ax->x_min, ax->y_min, ax->z_min}, {ax->x_max, ax->y_min, ax->z_min},
        {ax->x_max, ax->y_max, ax->z_min}, {ax->x_min, ax->y_max, ax->z_min},
        {ax->x_min, ax->y_min, ax->z_max}, {ax->x_max, ax->y_min, ax->z_max},
        {ax->x_max, ax->y_max, ax->z_max}, {ax->x_min, ax->y_max, ax->z_max}
    };

    float px[8], py[8];
    for (int i = 0; i < 8; i++) {
        project_3d(ax, corners[i][0], corners[i][1], corners[i][2], &px[i], &py[i]);
    }

    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Light gray cage
    
    // Connect bottom 4, top 4, and vertical 4
    for (int i = 0; i < 4; i++) {
        SDL_RenderLine(renderer, px[i], py[i], px[(i+1)%4], py[(i+1)%4]);         // Bottom
        SDL_RenderLine(renderer, px[i+4], py[i+4], px[((i+1)%4)+4], py[((i+1)%4)+4]); // Top
        SDL_RenderLine(renderer, px[i], py[i], px[i+4], py[i+4]);               // Verticals
    }
}