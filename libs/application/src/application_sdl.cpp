#include "common/config.hpp"

#if RN_PLATFORM_WINDOWS
#define SDL_MAIN_HANDLED
#include "app/application.hpp"
#include "app/render_window.hpp"
#include "SDL.h"

namespace rn::app
{
    void Application::InitializePlatform()
    {
        SDL_SetMainReady();
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
    }
    
    void Application::TeardownPlatform()
    {
        SDL_Quit();
    }

    bool Application::ProcessEvents()
    {
        SDL_PumpEvents();

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                return false;
            }
            else if (event.type == SDL_WINDOWEVENT)
            {
                SDL_Window* sdlWindow = SDL_GetWindowFromID(event.window.windowID);
                RenderWindow* renderWindow = (RenderWindow*)SDL_GetWindowData(sdlWindow, "RN_RENDER_WINDOW");
                if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    renderWindow->OnResize();   
                }
            }
        }

        return true;
    }
}

#endif // RN_PLATFORM_WINDOWS