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
                bool setup_failed = false;
                
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
                char* current_resolution_name = resolution_name_list[5];
                int backbuffer_width  = resolution_list[5][0];
                int backbuffer_height = resolution_list[5][1];
                
                float fov = PI32/2;
                
                GLuint display_vao;
                GLuint display_program;
                {
                    char* vertex_shader_code =
                        "#version 450\n"
                        "\n"
                        "out vec2 uv;\n"
                        "\n"
                        "void\n"
                        "main()\n"
                        "{\n"
                        "\tgl_Position.xy = vec2((gl_VertexID%2)*4, (gl_VertexID/2)*4) + vec2(-1,-1);\n"
                        "\tgl_Position.zw = vec2(0, 1);\n"
                        "\tuv             = vec2((gl_VertexID%2)*2, (gl_VertexID/2)*2);\n"
                        "}\n";
                    
                    char* fragment_shader_code =
                        "#version 450\n"
                        "\n"
                        "in vec2 uv;\n"
                        "\n"
                        "out vec4 color;\n"
                        "\n"
                        "layout(binding = 0) uniform sampler2D backbuffer;\n"
                        "\n"
                        "void\n"
                        "main()\n"
                        "{\n"
                        "\tcolor = vec4(texture(backbuffer, uv).rgb, 1.0);"
                        "}\n";
                    
                    glGenVertexArrays(1, &display_vao);
                    
                    display_program = glCreateProgram();
                    
                    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
                    glShaderSource(vertex_shader, 1, &vertex_shader_code, 0);
                    glCompileShader(vertex_shader);
                    
                    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
                    glShaderSource(fragment_shader, 1, &fragment_shader_code, 0);
                    glCompileShader(fragment_shader);
                    
                    glAttachShader(display_program, vertex_shader);
                    glAttachShader(display_program, fragment_shader);
                    glLinkProgram(display_program);
                    
                    glDeleteShader(vertex_shader);
                    glDeleteShader(fragment_shader);
                }
                
                GLuint compute_program = 0;
                {
                    FILE* compute_shader_file = fopen("../src/compute_shader.comp", "r");
                    if (compute_shader_file == 0)
                    {
                        fprintf(stderr, "ERROR: failed to open compute shader file.\n");
                        setup_failed = true;
                    }
                    else
                    {
                        fseek(compute_shader_file, 0, SEEK_END);
                        int compute_shader_file_size = ftell(compute_shader_file);
                        rewind(compute_shader_file);
                        
                        char* compute_shader_code = (char*)malloc(compute_shader_file_size + 1);
                        memset(compute_shader_code, 0, compute_shader_file_size + 1);
                        
                        if (compute_shader_code == 0 || fread(compute_shader_code, 1, compute_shader_file_size, compute_shader_file) != compute_shader_file_size)
                        {
                            fprintf(stderr, "ERROR: failed to read compute shader file.\n");
                            setup_failed = true;
                        }
                        else
                        {
                            do
                            {
                                GLuint compute_shader = glCreateShader(GL_COMPUTE_SHADER);
                                glShaderSource(compute_shader, 1, &compute_shader_code, 0);
                                glCompileShader(compute_shader);
                                
                                GLint status;
                                glGetShaderiv(compute_shader, GL_COMPILE_STATUS, &status);
                                if (!status)
                                {
                                    char buffer[1024];
                                    glGetShaderInfoLog(compute_shader, sizeof(buffer), 0, buffer);
                                    fprintf(stderr, "%s\n", buffer);
                                    setup_failed = true;
                                    break;
                                }
                                
                                compute_program = glCreateProgram();
                                glAttachShader(compute_program, compute_shader);
                                glLinkProgram(compute_program);
                                
                                glGetProgramiv(compute_program, GL_LINK_STATUS, &status);
                                if (!status)
                                {
                                    char buffer[1024];
                                    glGetProgramInfoLog(compute_program, sizeof(buffer), 0, buffer);
                                    fprintf(stderr, "%s\n", buffer);
                                    setup_failed = true;
                                    break;
                                }
                                
                                glValidateProgram(compute_program);
                                glGetProgramiv(compute_program, GL_VALIDATE_STATUS, &status);
                                if (!status)
                                {
                                    char buffer[1024];
                                    glGetProgramInfoLog(compute_program, sizeof(buffer), 0, buffer);
                                    fprintf(stderr, "%s\n", buffer);
                                    setup_failed = true;
                                    break;
                                }
                            } while (0);
                        }
                        
                        free(compute_shader_code);
                        fclose(compute_shader_file);
                    }
                }
                
                if (!setup_failed)
                {
                    GLuint backbuffer_texture = 0;
                    
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
                                    backbuffer_width  = resolution_list[i][0];
                                    backbuffer_height = resolution_list[i][1];
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
                        
                        if (backbuffer_texture != 0) glDeleteTextures(1, &backbuffer_texture);
                        glGenTextures(1, &backbuffer_texture);
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, backbuffer_texture);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, backbuffer_width, backbuffer_height, 0, GL_RGBA, GL_FLOAT, 0);
                        
                        glBindImageTexture(0, backbuffer_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
                        
                        glUseProgram(compute_program);
                        glUniform1f(0, fov);
                        glDispatchCompute(backbuffer_width, backbuffer_height, 1);
                        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
                        
                        glBindVertexArray(display_vao);
                        glUseProgram(display_program);
                        glDrawArrays(GL_TRIANGLES, 0, 3);
                        glBindVertexArray(0);
                        
                        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                        
                        SDL_GL_SwapWindow(window);
                    }
                }
                
                ImGui_ImplOpenGL3_Shutdown();
                ImGui_ImplSDL2_Shutdown();
                ImGui::DestroyContext();
            }
            
            SDL_GL_DeleteContext(gl_context);
            SDL_DestroyWindow(window);
        }
    }
    
    SDL_Quit();
    
    return 0;
}