#include <stdio.h>

#include "GL/glew.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#define PI32 3.1415926535f

void
GLDebugProc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    (void)source;
    (void)type;
    (void)id;
    (void)severity;
    (void)length;
    (void)userParam;
    
    fprintf(stderr, "%s\n", message);
}

int
main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) fprintf(stderr, "ERROR: failed to initialize sdl2. %s\n", SDL_GetError());
    else
    {
        const char* glsl_version = "#version 450";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
        
        SDL_Window* window = SDL_CreateWindow("TDT4230 Project", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP|SDL_WINDOW_OPENGL|SDL_WINDOW_ALLOW_HIGHDPI);
        if (window == 0) fprintf(stderr, "ERROR: failed to create window. %s\n", SDL_GetError());
        else
        {
            SDL_GLContext gl_context = SDL_GL_CreateContext(window);
            GLenum glew_error;
            if      (gl_context == 0)                             fprintf(stderr, "ERROR: failed create OpenGL context. %s\n", SDL_GetError());
            else if (SDL_GL_MakeCurrent(window, gl_context) != 0) fprintf(stderr, "ERROR: failed to make OpenGL context current. %s\n", SDL_GetError());
            else if ((glew_error = glewInit()) != GLEW_OK)        fprintf(stderr, "ERROR: failed initialize OpenGL extension loader. %s\n", glewGetErrorString(glew_error));
            else
            {
                SDL_GL_SetSwapInterval(1);
                
                glEnable(GL_DEBUG_OUTPUT);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                glDebugMessageCallback(GLDebugProc, 0);
                
                IMGUI_CHECKVERSION();
                ImGui::CreateContext();
                ImGuiIO& io = ImGui::GetIO();
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
                io.IniFilename = 0;
                
                ImGui::StyleColorsDark();
                
                ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
                ImGui_ImplOpenGL3_Init(glsl_version);
                
                char* resolution_name_list[] = {
                    "256x144",
                    "320x180   QnHD",
                    "426x240   NTSC widescreen",
                    "640x360   nHD",
                    "848x480",
                    "854x480   FWVGA",
                    "960x540   qHD",
                    "1024x576",
                    "1280x720  HD",
                    "1366x768",
                    "1600x900  HD+",
                    "1920x1080 Full HD",
                    "2560x1440 QHD",
                    "3200x1800 QHD+",
                    "3840x2160 4K UHD",
                    "5120x2880 5K",
                    "7680x4320 8K UHD"
                };
                
                int resolution_list[][2] = {
                    {256,  144},
                    {320,  180},
                    {426,  240},
                    {640,  360},
                    {848,  480},
                    {854,  480},
                    {960,  540},
                    {1024, 576},
                    {1280, 720},
                    {1366, 768},
                    {1600, 900},
                    {1920, 1080},
                    {2560, 1440},
                    {3200, 1800},
                    {3840, 2160},
                    {5120, 2880},
                    {7680, 4320}
                };
                
                // 1920x1080
                char* current_resolution_name = resolution_name_list[11];
                int image_width  = resolution_list[11][0];
                int image_height = resolution_list[11][1];
                
                float fov = PI32/2;
                
                GLuint default_vao;
                GLuint program;
                {
                    char* vertex_shader_code =
                        "#version 450\n"
                        "\n"
                        "void\n"
                        "main()\n"
                        "{\n"
                        "\tgl_Position.xy = vec2((gl_VertexID/2)*4, -(gl_VertexID%2)*4) + vec2(-1,1);\n"
                        "\tgl_Position.zw = vec2(0, 1);\n"
                        "}\n";
                    
                    char* fragment_shader_code =
                        "#version 450\n"
                        "\n"
                        "out vec4 color;\n"
                        "\n"
                        "void\n"
                        "main()\n"
                        "{\n"
                        "\tcolor = vec4(1, 0, 1, 1);"
                        "}\n";
                    
                    glGenVertexArrays(1, &default_vao);
                    
                    program = glCreateProgram();
                    
                    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
                    glShaderSource(vertex_shader, 1, &vertex_shader_code, 0);
                    glCompileShader(vertex_shader);
                    
                    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
                    glShaderSource(fragment_shader, 1, &fragment_shader_code, 0);
                    glCompileShader(fragment_shader);
                    
                    glAttachShader(program, vertex_shader);
                    glAttachShader(program, fragment_shader);
                    glLinkProgram(program);
                    
                    glDeleteShader(vertex_shader);
                    glDeleteShader(fragment_shader);
                }
                
                bool done = false;
                while (!done)
                {
                    SDL_Event event;
                    while (SDL_PollEvent(&event))
                    {
                        ImGui_ImplSDL2_ProcessEvent(&event);
                        if (event.type == SDL_QUIT) done = true;
                    }
                    
                    ImGui_ImplOpenGL3_NewFrame();
                    ImGui_ImplSDL2_NewFrame();
                    ImGui::NewFrame();
                    
                    int window_width;
                    int window_height;
                    SDL_GetWindowSize(window, &window_width, &window_height);
                    ImGui::SetNextWindowPos(ImVec2((1 - 0.15f)*window_width, 0));
                    ImGui::SetNextWindowSize(ImVec2(0.15f*window_width, (float)window_height));
                    ImGui::Begin("Properties", 0, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);
                    
                    if (ImGui::BeginCombo("Resolution", current_resolution_name))
                    {
                        for (int i = 0; i < IM_ARRAYSIZE(resolution_name_list); ++i)
                        {
                            if (ImGui::Selectable(resolution_name_list[i], resolution_name_list[i] == current_resolution_name))
                            {
                                current_resolution_name = resolution_name_list[i];
                                image_width  = resolution_list[i][0];
                                image_height = resolution_list[i][1];
                            }
                            
                            if (resolution_name_list[i] == current_resolution_name)
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        
                        ImGui::EndCombo();
                    }
                    
                    ImGui::SliderFloat("Fov", &fov, 0.1f, 0.9f*PI32, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                    ImGui::End();
                    
                    ImGui::Render();
                    
                    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
                    glClearColor(0, 0, 0, 1);
                    glClear(GL_COLOR_BUFFER_BIT);
                    
                    glBindVertexArray(default_vao);
                    glUseProgram(program);
                    glDrawArrays(GL_TRIANGLES, 0, 3);
                    glBindVertexArray(0);
                    
                    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                    
                    SDL_GL_SwapWindow(window);
                }
            }
            
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplSDL2_Shutdown();
            ImGui::DestroyContext();
            
            SDL_GL_DeleteContext(gl_context);
            SDL_DestroyWindow(window);
        }
    }
    
    SDL_Quit();
    
    return 0;
}