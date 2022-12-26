#define SDL_MAIN_HANDLED
#include <SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Test.h"

static int graphics_init(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **texture,const char *f)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0)
    {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return -1;
    }

    *window = SDL_CreateWindow("SDL is active!", 100, 100, 500, 500, 0);
    if (!*window)
    {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!*renderer){
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(*window);
        SDL_Quit();
        return -1;
    }
    

    int width;
    int height;
    int channels;
    //unsigned char *pixels = stbi_load(f, &width, &height, &channels,4);
    unsigned char *pixels = parse_png(f, &width, &height, &channels);
    if (!pixels)
    {
        SDL_Log("Unable to open image");
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        return -1;
    }
    SDL_Log("Image width: %d height: %d channels: %d", width, height, channels);


    *texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
    if (!*texture)    
    {
        SDL_Log("Unable to create texture: %s", SDL_GetError());
        free(pixels);
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        return -1;
    }

    SDL_UpdateTexture(*texture, NULL, pixels, width * 4);
    SDL_SetTextureAlphaMod(*texture, 255);
    SDL_SetTextureBlendMode(*texture, SDL_BLENDMODE_BLEND);
    free(pixels);
    return 0;
}

int main(int argc, char **argv)
{

    if (argc < 2)
    {
        printf("Please write png filename");
        return -1;
    }


    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    int ret = graphics_init(&window, &renderer, &texture, argv[1]);
    if (ret < 0)
    {
        SDL_Quit();
        return ret;
    }
    

    // game loop
    int running = 1;
    
    float performance_counter_new = (float)SDL_GetPerformanceCounter();
    float performance_counter_old = 0;
    float delta_time = 0;
    while (running)
    {

        double performance_counter_new = (float)SDL_GetPerformanceCounter();
        delta_time = (performance_counter_new - performance_counter_old)*100 / (float)SDL_GetPerformanceFrequency();
        //delta_time = (float)((NOW - LAST)*1000 / (float)SDL_GetPerformanceFrequency() );
        //printf("%f \n",delta_time);
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = 0;
            }
        }
        SDL_PumpEvents();
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Rect target_rect = {100, 100, 100, 100};
        SDL_RenderCopy(renderer, texture, NULL, &target_rect);

        SDL_RenderPresent(renderer);
        performance_counter_old = performance_counter_new;
    }
    return 0;
}
