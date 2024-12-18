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
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
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
            if (_eventListenerHook)
            {
                _eventListenerHook(&event);
            }

            if (event.type == SDL_QUIT)
            {
                return false;
            }
            else if (event.type == SDL_WINDOWEVENT)
            {
                SDL_Window* sdlWindow = SDL_GetWindowFromID(event.window.windowID);
                RenderWindow* renderWindow = (RenderWindow*)SDL_GetWindowData(sdlWindow, "RN_RENDER_WINDOW");
                if (renderWindow && (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
                {
                    renderWindow->OnResize();
                }
            }
        }

        return true;
    }
}

#endif // RN_PLATFORM_WINDOWS