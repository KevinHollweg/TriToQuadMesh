#pragma once

#include "config.h"
#include "shaderManager.h"
#include "imgui/imgui_impl_sdl_gl3.h"

class Window
{
public:
    Window(int w, int h);
    virtual ~Window();

    void startMainLoop();

    virtual void update(float dt) {}
    virtual void render() {}
    virtual void renderGui() {}
protected:
    SDL_Window* sdlWindow;
    SDL_GLContext context;


    ImGui_SDL_Renderer imgui;
    int w;
    int h;
    ShaderManager shaderManager;
};

