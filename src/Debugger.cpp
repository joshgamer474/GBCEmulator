#include <Debugger.h>
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>

Debugger::Debugger(SDL_GLContext* _glContext, SDL_Window* _window)
{
    glContext = _glContext;
    window = _window;

    draw_vram_debugger = false;
    draw_audio_debugger = false;
    draw_register_debugger = false;


    bool err = false;
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    //asdf
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    err = glewInit();
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    //asdf
#else
    //asdf
#endif

    if (err)
    {
        return;
    }


    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, *glContext);
    ImGui_ImplOpenGL3_Init("#version 130");
}

Debugger::~Debugger()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void Debugger::draw(SDL_Event* event, SDL_Renderer* renderer, SDL_Texture* screen_texture)
{
    if (event)
    {
        ImGui_ImplSDL2_ProcessEvent(event);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();

    //bool showDemoWindow = true;
    //ImGui::ShowDemoWindow(&showDemoWindow);

    // Generate ImGui windows
    ImGui::Begin("Hello world!");
    ImGui::Checkbox("Show VRAM debugger", &draw_vram_debugger);
    ImGui::Checkbox("Show APU debugger", &draw_audio_debugger);
    ImGui::Checkbox("Show Register debugger", &draw_register_debugger);
    ImGui::End();

    if (draw_vram_debugger)
    {

    }

    if (draw_audio_debugger)
    {

    }

    if (draw_register_debugger)
    {
        ImGui::Begin("Register Debugger");
        if (emu)
        {
            ImGui::LabelText("BC", "%X", emu->get_CPU()->get_register_16(CPU::REGISTERS::BC));
            ImGui::LabelText("DE", "%X", emu->get_CPU()->get_register_16(CPU::REGISTERS::DE));
            ImGui::LabelText("HL", "%X", emu->get_CPU()->get_register_16(CPU::REGISTERS::HL));
            ImGui::LabelText("AF", "%X", emu->get_CPU()->get_register_16(CPU::REGISTERS::AF));
            ImGui::LabelText("SP", "%X", emu->get_CPU()->get_register_16(CPU::REGISTERS::SP));
            ImGui::LabelText("PC", "%X", emu->get_CPU()->get_register_16(CPU::REGISTERS::PC));
        }
        ImGui::End();
    }

    // Start drawing ImGui windows
    //glUseProgram(0);

    //SDL_RenderPresent(renderer);
    ImGui::Render();
    //SDL_GL_MakeCurrent(window, glContext);
    if (!emu)
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
}

void Debugger::setEmulator(std::shared_ptr<GBCEmulator> _emu)
{
    emu = _emu;
}