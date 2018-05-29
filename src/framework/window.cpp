#include "window.h"


Window::Window(int w, int h)
    : w(w), h(h) ,  shaderManager("shaders/", ".glsl", true)
{

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        std::cerr << "SDL_Init() failed" << std::endl;
        assert(0);
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    Uint32 flags =  SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
    sdlWindow = SDL_CreateWindow("CG", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, flags );
    assert(sdlWindow);

    context = SDL_GL_CreateContext( sdlWindow );
    assert(context);

    // Setup glew
    if (glewInit() != GLEW_OK) {
        std::cerr << "glewInit() failed." << std::endl;
        SDL_Quit();
        assert(0);
    }

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;


    imgui.init(sdlWindow);
}

Window::~Window()
{

    SDL_Quit();
}

void Window::startMainLoop()
{
    int startTime = SDL_GetTicks();
    int previousTime = startTime;

    // Mainloop
    bool running = true;
    while (running) {
        int currentTime = SDL_GetTicks() - startTime;

        SDL_Event event;
        while (SDL_PollEvent(&event) > 0) {
            imgui.processEvent(event);
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                running = false;
                break;
            }
        }

        if (!running) break;

        float dt = (currentTime - previousTime) / 1000.0f;

        update(dt);
        render();

        imgui.beginFrame();
        renderGui();
        imgui.endFrame();

        SDL_GL_SwapWindow( sdlWindow );

        previousTime = currentTime;
    }

}
