#ifndef SDLGRAPHS_H
#define SDLGRAPHS_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

typedef enum {
    PROJECTION_2D=0,
    PROJECTION_3D
} ProjectionType;

typedef enum {
    STYLE_SOLID,
    STYLE_DASHED,
    STYLE_DOTTED
} LineStyle;

typedef enum {
    PLOT_LINE,
    PLOT_SCATTER
} PlotType;

typedef struct {
    float* x;
    float* y;
    float* z; //Getting into the z axis wooo
    int count;
    SDL_Color color;
    PlotType type;    // Added to distinguish between lines and dots
    LineStyle style;
    float marker_size; // Control how big the scatter points are
    float thickness;
    char label[32]; // The name of this line (e.g., "Sensor A")
} Series;

// The "Axes" - handles coordinates and drawing
typedef struct {
    SDL_FRect rect;       // Position on screen
    ProjectionType projection;
    Series* lines;
    int line_count;
    // Titles
    const char* title;
    const char* x_label;
    const char* y_label;
    const char* z_label;
    float x_min, x_max;   // Data limits
    float y_min, y_max;
    float z_min, z_max;
    // Position relative to window (0.0 to 1.0)
    float rel_x, rel_y, rel_w, rel_h; 
    bool show_grid;
    bool show_legend;
    // Camera state for 3D
    float phi;   // Azimuthal angle (rotation around Z)
    float theta; // Elevation angle (angle from Z-axis)
    float zoom;
} Axes;

// The "Figure" - the top level container
typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    Axes* axes;
    int axes_count;
    void* toolbar;
} Figure;

Figure* subplots(const char* title, int width, int height,int num_axes);
void plot(Axes* ax, float* x, float* y, int count, SDL_Color color);
void scatter(Axes* ax, float* x, float* y, int count, SDL_Color color, float size);
void render_axes(SDL_Renderer* renderer, TTF_Font* font, Axes* ax);
void draw_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, float x, float y, bool right_align, SDL_Color color);
void update_layout(Figure* fig, int window_w, int window_h);
void set_grid(Axes* ax, bool enabled);
void SDL_RenderLineDashed(SDL_Renderer* renderer, float x1, float y1, float x2, float y2, LineStyle style);
void set_linestyle(Axes* ax, int series_idx, LineStyle style);
void RenderThickLine(SDL_Renderer* renderer, float x1, float y1, float x2, float y2, float thickness); // Now redundant I think
void DrawDashedThickLine(SDL_Renderer* renderer, float x1, float y1, float x2, float y2, float thickness, LineStyle style);
void render_legend(SDL_Renderer* renderer, TTF_Font* font, Axes* ax);
void set_legend(Axes* ax, bool enabled);
void set_label(Axes* ax, int series_idx, const char* name);
void set_xlabel(Axes* ax, const char* label);
void set_ylabel(Axes* ax, const char* label);
void set_title(Axes* ax, const char* title);
void destroy_figure(Figure* fig);
void show(Figure* fig);
void save_figure_as_png(Figure* fig, const char* filename);
void plot3D(Axes* ax, float* x, float* y, float* z, int count, SDL_Color color);
void set_projection(Axes* ax, ProjectionType proj);
void render_axes_2d(SDL_Renderer* renderer, TTF_Font* font, Axes* ax);
void render_axes_3d(SDL_Renderer* renderer, TTF_Font* font, Axes* ax);
static void project_3d(Axes* ax, float x, float y, float z, float* px, float* py);
void draw_3d_box(SDL_Renderer* renderer, Axes* ax);

#endif