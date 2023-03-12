#include <SDL.h>
#include <stdio.h>

int
main(int argc, char** args)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) fprintf(stderr, "ERROR: failed to initialize sdl2. %s\n", SDL_GetError());
    else
    {
        SDL_Window* window = SDL_CreateWindow("TDT4230 Project", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP|SDL_WINDOW_OPENGL);
        if (window == 0) fprintf(stderr, "ERROR: failed to create window. %s\n", SDL_GetError());
        else
        {
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
            
            SDL_Surface* surface = SDL_GetWindowSurface(window);
            
            SDL_FillRect(surface, 0, SDL_MapRGB(surface->format, 0xFF, 0x00, 0xFF));
            
            SDL_UpdateWindowSurface(window);
            SDL_Delay(5000);
        }
    }
    
    SDL_Quit();
    return 0;
}