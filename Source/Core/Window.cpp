//
// Created by Maddie on 6/18/2024.
//

#include <Core/Window.h>
#include <Core/Console.h>
#include <Editor/EditorSystem.h>

#include <gl/glew.h>

using namespace OpenShaderDesigner;

Window::Window(const Configuration& config)
    : Config(config)
{
    int flags = static_cast<int>(Config.Video.Fullscreen) | SDL_WINDOW_OPENGL;
    flags |= Config.Video.Fullscreen == FullscreenMode::WINDOWED ? SDL_WINDOW_RESIZABLE : 0;

    SDL_Init(SDL_INIT_EVERYTHING & ~SDL_INIT_AUDIO);
    if((Handle = SDL_CreateWindow(
        Config.Application.Title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        Config.Video.Resolution.x, Config.Video.Resolution.y,
        flags)) == nullptr)
    {
        Console::Log(Console::Severity::FATAL, "Failed to create SDL Window: {}", SDL_GetError());
        assert(false);
        return;
    }

#ifdef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG | SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif

    // Set OpenGL APIVersion
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    if(Config.Video.HDR)
    {
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 16);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 16);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 16);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 16);
    }

    Context = SDL_GL_CreateContext(Handle);

    if(Context == nullptr)
    {
        Console::Log(Console::Severity::FATAL, "Failed to create OpenGL Context: {}", SDL_GetError());
        SDL_DestroyWindow(Handle);
        assert(false);
        return;
    }

    if(SDL_GL_MakeCurrent(Handle, Context))
    {
        Console::Log(Console::Severity::FATAL, "Failed to set OpenGL Context: {}", SDL_GetError());
        SDL_GL_DeleteContext(Context);
        SDL_DestroyWindow(Handle);
        assert(false);
        return;
    }

    // Set VSync Mode
    SDL_GL_SetSwapInterval(static_cast<int>(Config.Video.VSync));

    // Enable experimental features
    glewExperimental = GL_TRUE;

    // Initialize GLEW
    GLenum error;
    if((error = glewInit()) != GLEW_OK)
    {
        Console::Log(Console::Severity::FATAL, "Failed to Initialize GLEW: {}", reinterpret_cast<const char*>(glewGetErrorString(error)));
        SDL_GL_DeleteContext(Context);
        SDL_DestroyWindow(Handle);
        assert(false);
        return;
    }

    // Fill in black screen
    glDisable(GL_DEPTH_TEST);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(Handle);

    // Set running
    Open = true;
}

Window::~Window()
{
    SDL_GL_DeleteContext(Context);
    SDL_DestroyWindow(Handle);
    SDL_Quit();
}

void Window::HandleEvents()
{
    SDL_Event event;

    // Poll for an event from SDL and check if it was received
    while (SDL_PollEvent(&event))
    {
        // if event was a quit event, stop the system
        if (event.type == SDL_QUIT)
        {
            Close();
            return;
        }


        if (event.type == SDL_WINDOWEVENT)
        {
            switch(event.window.event)
            {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
                Config.Video.Resolution.x = event.window.data1;
                Config.Video.Resolution.y = event.window.data2;
                break;
            default:
                break;
            }
        }

        EditorSystem::HandleEvents(&event);

        SDLEvent sdlEvent(event);
        EventSystem::PostEvent(&sdlEvent);
    }
}

void Window::BeginFrame()
{
    OpenShaderDesigner::BeginFrame event;
    EventSystem::PostEvent(&event);

    HandleEvents();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, Config.Video.Resolution.x, Config.Video.Resolution.y);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Window::EndFrame()
{
    SDL_GL_SwapWindow(Handle);

    OpenShaderDesigner::EndFrame event;
    EventSystem::PostEvent(&event);
}