#include "sdl_graphs.h"
#include <math.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO) || TTF_Init() < 0) return -1;

    Figure* fig = subplots("3D Spiral Demo", 800, 600, 1);
    
    // Create data for 100 points
    int n = 100;
    float x[100], y[100], z[100];

    for (int i = 0; i < n; i++) {
        float t = (float)i / n;           // 0.0 to 1.0
        float angle = t * 2.0f * M_PI * 4.0f; // 4 full rotations
        
        x[i] = t * cosf(angle);           // Radius grows with t
        y[i] = t * sinf(angle);
        z[i] = t;                         // Height grows with t
    }

    // This will automatically call set_projection(..., PROJECTION_3D)
    plot3D(&fig->axes[0], x, y, z, n, (SDL_Color){50, 255, 100, 255});
    set_title(&fig->axes[0], "3D Parametric Spiral");

    show(fig);

    TTF_Quit();
    SDL_Quit();
    return 0;
}