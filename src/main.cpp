#include <stdio.h>
#include <stdint.h>
#include <functional>

#include "GL/glew.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#define PI32 3.1415926535f

#define ASSERT(EX) ((EX) ? 1 : (*(volatile int*)0 = 0), 0)

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#define CONCAT_(X, Y) X##Y
#define CONCAT(X, Y) CONCAT_(X, Y)

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define DEFER(STATEMENT) DeferStatement CONCAT(defer_statement, CONCAT(__LINE__, CONCAT(_, __COUNTER__)))([&]{STATEMENT;})
class DeferStatement
{
    std::function<void()> lambda;
    
    public:
    DeferStatement(std::function<void()> lambda)
    {
        this->lambda = lambda;
    }
    
    ~DeferStatement()
    {
        this->lambda();
    }
};

#if defined(_WIN32) && defined(_WIN64)

#define STRICT
#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX            1

#include <windows.h>
#include <timeapi.h>

#undef STRICT
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX
#undef far
#undef near

u64
GetTicks()
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result.QuadPart;
}

float
DiffTicksInMs(u64 start, u64 end)
{
    static float res_perf_freq = 0;
    if (res_perf_freq == 0)
    {
        LARGE_INTEGER perf_freq;
        QueryPerformanceFrequency(&perf_freq);
        res_perf_freq = 1.0f / (float)perf_freq.QuadPart;
    }
    
    return res_perf_freq*(1000*(end - start));
}
#elif defined(__unix__)
#include <time.h>
u64
GetTicks()
{
    struct timespec result;
    clock_gettime(CLOCK_REALTIME, &result);
    return (u64)result.tv_nsec;
}

float
DiffTicksInMs(u64 start, u64 end)
{
    return (float)(end - start) / 1000;
}
#else
#error Unsupported platform
#endif

void
GLDebugProc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    (void)source;
    (void)type;
    (void)id;
    (void)length;
    (void)userParam;
    
    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
    {
        fprintf(stderr, "%s\n", message);
    }
}

char* ResolutionNames[] = {
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

int Resolutions[][2] = {
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

struct State
{
    float fov;
    
    int current_resolution_index;
    int backbuffer_width;
    int backbuffer_height;
    
    GLuint display_vao;
    GLuint display_program;
    
    GLuint compute_program;
    GLuint backbuffer_texture;
    GLuint accumulated_frames_texture;
		GLuint triangle_buffer;
    
    bool should_regen_buffers;
    u32 frame_index;
    
    u64 last_render_timestamp;
    float last_render_time;
};

struct Triangle
{
	float alpha[4];
	float beta[4];
	float gamma;
};

int
main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    
    DEFER(SDL_Quit());
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) fprintf(stderr, "ERROR: failed to initialize sdl2. %s\n", SDL_GetError());
    else
    {
        const char* glsl_version = "#version 450";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
        
        SDL_Window* window = SDL_CreateWindow("TDT4230 Project", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP|SDL_WINDOW_OPENGL);
        DEFER(SDL_DestroyWindow(window));
        if (window == 0) fprintf(stderr, "ERROR: failed to create window. %s\n", SDL_GetError());
        else
        {
            SDL_GLContext gl_context = SDL_GL_CreateContext(window);
            DEFER(SDL_GL_DeleteContext(gl_context));
            
            GLenum glew_error;
            if      (gl_context == 0)                             fprintf(stderr, "ERROR: failed create OpenGL context. %s\n", SDL_GetError());
            else if (SDL_GL_MakeCurrent(window, gl_context) != 0) fprintf(stderr, "ERROR: failed to make OpenGL context current. %s\n", SDL_GetError());
            else if ((glew_error = glewInit()) != GLEW_OK)        fprintf(stderr, "ERROR: failed initialize OpenGL extension loader. %s\n", glewGetErrorString(glew_error));
            else
            {
                SDL_GL_SetSwapInterval(0);
                glEnable(GL_DEBUG_OUTPUT);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                glDebugMessageCallback(GLDebugProc, 0);
                
                /// ImGui setup and teardown
                IMGUI_CHECKVERSION();
                ImGui::CreateContext();
                ImGuiIO& io = ImGui::GetIO();
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
                io.IniFilename = 0;
                
                ImGui::StyleColorsDark();
                
                ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
                ImGui_ImplOpenGL3_Init(glsl_version);
                DEFER({
                          ImGui_ImplOpenGL3_Shutdown();
                          ImGui_ImplSDL2_Shutdown();
                          ImGui::DestroyContext();
                      });
                
                
                State state = {};
                state.fov                      = PI32/2;
                state.current_resolution_index = 5;
                state.backbuffer_width         = Resolutions[state.current_resolution_index][0];
                state.backbuffer_height        = Resolutions[state.current_resolution_index][1];
                state.should_regen_buffers     = true;
                
                /// Program setup
                bool setup_failed = false;
                { 
                    /// Create program and vertex array for displaying backbuffer on screen
                    glGenVertexArrays(1, &state.display_vao);
                    state.display_program = glCreateProgram();
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
                        
                        GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
                        glShaderSource(vertex_shader, 1, &vertex_shader_code, 0);
                        glCompileShader(vertex_shader);
                        
                        GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
                        glShaderSource(fragment_shader, 1, &fragment_shader_code, 0);
                        glCompileShader(fragment_shader);
                        
                        glAttachShader(state.display_program, vertex_shader);
                        glAttachShader(state.display_program, fragment_shader);
                        glLinkProgram(state.display_program);
                        
                        glDeleteShader(vertex_shader);
                        glDeleteShader(fragment_shader);
                    }

                    /// Generate textures
                    {
                        glActiveTexture(GL_TEXTURE0);
                        glGenTextures(1, &state.backbuffer_texture);
                        glBindTexture(GL_TEXTURE_2D, state.backbuffer_texture);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, state.backbuffer_width, state.backbuffer_height, 0, GL_RGBA, GL_FLOAT, 0);
                        
                        glActiveTexture(GL_TEXTURE1);
                        glGenTextures(1, &state.accumulated_frames_texture);
                        glBindTexture(GL_TEXTURE_2D, state.accumulated_frames_texture);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, state.backbuffer_width, state.backbuffer_height, 0, GL_RGBA, GL_FLOAT, 0);
                    }
                    
										/// Create shader buffers for storing scene data
										{
											unsigned int triangle_count = 100000;
											Triangle* triangles = (Triangle*)malloc(sizeof(Triangle)*triangle_count);
											DEFER(free(triangles));

											float screen_width = 1;
											float screen_height = 9.0f/16.0f;

											float p[3][3];
											p[0][0] = -screen_width*2;
											p[0][1] = -screen_height/2;
											p[0][2] = 0.5f;
											p[1][0] = screen_width/2;
											p[1][1] = -screen_height/2;
											p[1][2] = 0.5f;
											p[2][0] = screen_width/2;
											p[2][1] = screen_height*2;
											p[2][2] = 0.5f;

											Triangle template_triangle = {};
											template_triangle.alpha[0] = p[0][0];
											template_triangle.alpha[1] = p[0][1];
											template_triangle.alpha[2] = p[0][2];
											template_triangle.beta[0]  = p[1][0];
											template_triangle.beta[1]  = p[1][1];
											template_triangle.beta[2]  = p[1][2];

											template_triangle.alpha[3] = p[2][0];
											template_triangle.beta[3]  = p[2][1];
											template_triangle.gamma    = p[2][2];

											for (unsigned int i = 0; i < triangle_count; ++i)
											{
												memcpy(&triangles[i], &template_triangle, sizeof(Triangle));
											}

											glGenBuffers(1, &state.triangle_buffer);
											glBindBuffer(GL_SHADER_STORAGE_BUFFER, state.triangle_buffer);
											//glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Triangle)*triangle_count, triangles, GL_STATIC_DRAW);
											glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(Triangle)*triangle_count, triangles, 0);
											glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, state.triangle_buffer);
											glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
										}

                    /// Create compute program for rendering to the backbuffer
                    state.compute_program = glCreateProgram();
                    {
                        FILE* compute_shader_file = fopen("../src/compute_shader.comp", "rb");
                        FILE* pcg_file = fopen("../vendor/pcg/pcg.comp", "rb");
                        if (compute_shader_file == 0)
                        {
                            fprintf(stderr, "ERROR: failed to open compute shader file.\n");
                            setup_failed = true;
                        }
												else if (pcg_file == 0)
												{
														fprintf(stderr, "ERROR: failed to open pcg shader file.\n");
														setup_failed = true;
												}
                        else
                        {
                            fseek(compute_shader_file, 0, SEEK_END);
                            int compute_shader_file_size = ftell(compute_shader_file);
                            rewind(compute_shader_file);

                            fseek(pcg_file, 0, SEEK_END);
                            int pcg_file_size = ftell(pcg_file);
                            rewind(pcg_file);
                            
                            char* compute_shader_code = (char*)malloc(compute_shader_file_size + 1);
                            memset(compute_shader_code, 0, compute_shader_file_size + 1);

                            char* pcg_code = (char*)malloc(pcg_file_size + 1);
                            memset(pcg_code, 0, pcg_file_size + 1);
                            
                            if (compute_shader_code == 0 || fread(compute_shader_code, 1, compute_shader_file_size, compute_shader_file) != compute_shader_file_size)
                            {
                                fprintf(stderr, "ERROR: failed to read compute shader file.\n");
                                setup_failed = true;
                            }
														else if (pcg_code == 0 || fread(pcg_code, 1, pcg_file_size, pcg_file) != pcg_file_size)
                            {
                                fprintf(stderr, "ERROR: failed to read pcg shader file.\n");
                                setup_failed = true;
                            }
                            else
                            {
                                do
                                {
                                    GLuint compute_shader = glCreateShader(GL_COMPUTE_SHADER);
                                    DEFER(glDeleteShader(compute_shader));
                                    
																		char* sources[2] = {
																			compute_shader_code,
																			pcg_code
																		};

                                    glShaderSource(compute_shader, ARRAY_SIZE(sources), sources, 0);
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
                                    
                                    glAttachShader(state.compute_program, compute_shader);
                                    glLinkProgram(state.compute_program);
                                    
                                    glGetProgramiv(state.compute_program, GL_LINK_STATUS, &status);
                                    if (!status)
                                    {
                                        char buffer[1024];
                                        glGetProgramInfoLog(state.compute_program, sizeof(buffer), 0, buffer);
                                        fprintf(stderr, "%s\n", buffer);
                                        setup_failed = true;
                                        break;
                                    }
                                    
                                    glValidateProgram(state.compute_program);
                                    glGetProgramiv(state.compute_program, GL_VALIDATE_STATUS, &status);
                                    if (!status)
                                    {
                                        char buffer[1024];
                                        glGetProgramInfoLog(state.compute_program, sizeof(buffer), 0, buffer);
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
                }
                
                if (!setup_failed)
                {
                    bool done = false;
                    while (!done)
                    {
                        SDL_Event event;
                        while (SDL_PollEvent(&event))
                        {
                            ImGui_ImplSDL2_ProcessEvent(&event);
                            if (event.type == SDL_QUIT) done = true;
                        }
                        
                        int window_width;
                        int window_height;
                        SDL_GetWindowSize(window, &window_width, &window_height);
                        
                        ImGui_ImplOpenGL3_NewFrame();
                        ImGui_ImplSDL2_NewFrame();
                        ImGui::NewFrame();
                        
                        ImGui::SetNextWindowPos(ImVec2((1 - 0.15f)*window_width, 0));
                        ImGui::SetNextWindowSize(ImVec2(0.15f*window_width, (float)window_height));
                        ImGui::Begin("Properties", 0, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);
                        
                        if (ImGui::BeginCombo("Resolution", ResolutionNames[state.current_resolution_index]))
                        {
                            for (int i = 0; i < IM_ARRAYSIZE(ResolutionNames); ++i)
                            {
                                if (ImGui::Selectable(ResolutionNames[i], i == state.current_resolution_index))
                                {
                                    state.current_resolution_index = i;
                                    state.backbuffer_width         = Resolutions[i][0];
                                    state.backbuffer_height        = Resolutions[i][1];
                                    
                                    state.should_regen_buffers = true;
                                }
                                
                                if (i == state.current_resolution_index)
                                {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }
                            
                            ImGui::EndCombo();
                        }
                        
                        if (ImGui::SliderFloat("Fov", &state.fov, 0.1f, 0.9f*PI32, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                        {
                            state.should_regen_buffers = true;
                        }
                        ImGui::Text("last render time: %.2f ms", state.last_render_time);
                        ImGui::End();
                        
                        glViewport(0, 0, window_width, window_height);
                        glClearColor(0, 0, 0, 1);
                        glClear(GL_COLOR_BUFFER_BIT);
                        
                        if (state.should_regen_buffers)
                        {
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, state.backbuffer_texture);
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, state.backbuffer_width, state.backbuffer_height, 0, GL_RGBA, GL_FLOAT, 0);
                            
                            glActiveTexture(GL_TEXTURE1);
                            glBindTexture(GL_TEXTURE_2D, state.accumulated_frames_texture);
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, state.backbuffer_width, state.backbuffer_height, 0, GL_RGBA, GL_FLOAT, 0);
                            float f[4] = {0, 0, 0, 0};
                            glClearTexImage(state.accumulated_frames_texture, 0, GL_RGBA, GL_FLOAT, f);
                            
                            glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

                            state.should_regen_buffers = false;
                            state.frame_index          = 0;
                        }
                        
                        glUseProgram(state.compute_program);
                        glBindImageTexture(0, state.backbuffer_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
                        glBindImageTexture(1, state.accumulated_frames_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
                        glUniform1ui(0, state.frame_index);
                        glUniform1f(1, state.fov);
                        glUniform2f(2, (float)state.backbuffer_width, (float)state.backbuffer_height);
                        
                        GLuint num_work_groups_x = state.backbuffer_width/32  + (state.backbuffer_width%32 != 0);
                        GLuint num_work_groups_y = state.backbuffer_height/32 + (state.backbuffer_height%32 != 0);
                        ASSERT(num_work_groups_x <= 65535 && num_work_groups_y <= 65535);
                        glDispatchCompute(num_work_groups_x, num_work_groups_y, 1);
                        
                        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
                        
                        glBindVertexArray(state.display_vao);
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, state.backbuffer_texture);
                        glUseProgram(state.display_program);
                        glDrawArrays(GL_TRIANGLES, 0, 3);
                        
                        ImGui::Render();
                        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                        
                        glFinish();
                        u64 current_timestamp = GetTicks();
                        state.last_render_time = DiffTicksInMs(state.last_render_timestamp, current_timestamp);
                        state.last_render_timestamp = current_timestamp;
                        
                        state.frame_index += 1;
                        
                        SDL_GL_SwapWindow(window);
                    }
                }
            }
        }
    }
    
    return 0;
}
