// dear imgui: standalone example application for SDL2 + OpenGL
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the example_sdl_opengl3/ folder**
// See imgui_impl_sdl.cpp for details.

#include <GL/glew.h>
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl2.h"
#include "utils.h"
#include "materials.h"
#include <stdio.h>
#include <cmath>
#include <SDL.h>
#include <SDL_opengl.h>
#include <GL/glu.h>
#include <GL/glut.h>

static void draw_coordinate();

// Degree to Radian
inline float D2R(float degree)
{
    return degree * M_PI / 180.0f;
}

// Main code
int main(int argc, const char* argv[])
{
    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "Error: %s\n", SDL_GetError());
        return -1;
    }

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Stanford Bunny", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1200, 600, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup GLEW
    //glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initalized GLEW\n");
        return -1;
    }

    // Init GLUT
    glutInit(&argc, (char**)argv);

    // 加载 Stanford Bunny 数据
    std::vector<GLfloat> vertices;
    std::vector<GLuint> faces;
    std::vector<GLfloat> normals;
    GLuint VBO, IBO, NBO;
    const char *filename = "bunny.obj";
    if (argc >= 2) {
        filename = argv[1];
    }
    load_bunny_data(filename, vertices, faces, normals);

    printf("%s loaded, vertices:%lu, faces:%lu, normals:%lu\n", filename, vertices.size() / 3, faces.size() / 3, normals.size() / 3);

    // 顶点缓冲区对象
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(GLfloat),
        vertices.data(),
        GL_STATIC_DRAW
    );
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // 顶点索引缓冲区对象
    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        faces.size() * sizeof(GLuint),
        faces.data(),
        GL_STATIC_DRAW
    );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // 法向量顶点缓冲区对象
    glGenBuffers(1, &NBO);
    glBindBuffer(GL_ARRAY_BUFFER, NBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        normals.size() * sizeof(GLfloat),
        normals.data(),
        GL_STATIC_DRAW
    );
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state
    GLfloat clear_color[4] = {0.34f, 0.82f, 0.82f, 1.00f};
    GLfloat global_ambient[4] = { 0.1, 0.1, 0.1, 0.0 };

    GLfloat light0_ambient[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    GLfloat light0_diffuse[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat light0_specular[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    GLfloat light1_ambient[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    GLfloat light1_diffuse[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat light1_specular[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    Material mat = materials[0];

    GLfloat light0_position[4] = { 1.0, 0.23, 0.23, 1.0 };
    GLfloat light1_position[4] = { -1.0f, -0.65f, 1.0f, 1.0f };

    bool draw_coord = false;
    bool draw_lights = false;
    bool enable_wire_view = false;

    float horizonal_angle = 45.0f;
    float pitch_angle = 60.0f;  // 与 y 轴正方向夹角，单位度

    // Main loop
    bool done = false;
    while (!done) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
        }

        // 自适应视口变换
        auto w = io.DisplaySize.x, h = io.DisplaySize.y;
        auto vw = std::min(w, h) > 800 ? 800 : std::min(w, h); // viewport width

        // 更新姿态
        if (ImGui::GetMousePos().x > w - vw) {
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                // 偏航角
                auto dl = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
                horizonal_angle += dl.x;
                ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
            }
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
                // 俯仰角
                auto dr = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
                pitch_angle -= dr.y;
                if (pitch_angle >= 360) {
                    pitch_angle -= 360;
                } else if (pitch_angle <= 0) {
                    pitch_angle += 360;
                }
                ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        // UI 设计代码
        {
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(340, 240), ImGuiCond_FirstUseEver);
            ImGui::Begin("Control");

            if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None)) {
                if (ImGui::BeginTabItem("Global")) {
                    ImGui::ColorEdit3("clear color", clear_color);
                    ImGui::ColorEdit3("global ambient", global_ambient);
                    ImGui::Checkbox("draw coordinate", &draw_coord);
                    ImGui::Checkbox("draw lights", &draw_lights);
                    ImGui::Checkbox("wire view", &enable_wire_view);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Material")) {
                    ImGui::ColorEdit4("ambient", mat.ambient);
                    ImGui::ColorEdit4("diffuse", mat.diffuse);
                    ImGui::ColorEdit4("specular", mat.specular);
                    ImGui::SliderFloat("shininess", &mat.shininess, 0, 128);
                    ImGui::Separator();
                    ImGui::Text("built-in materials");
                    int count = 1;
                    for (const Material& m : materials) {
                        if (ImGui::Button(m.name)) {
                            mat = m;
                        }
                        if (count % 5 != 0)
                            ImGui::SameLine();
                        count += 1;
                    }
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Light0")) {
                    ImGui::ColorEdit4("ambient", light0_ambient);
                    ImGui::ColorEdit4("diffuse", light0_diffuse);
                    ImGui::ColorEdit4("specular", light0_specular);
                    ImGui::Text("position:");
                    ImGui::SliderFloat("x", &light0_position[0], -5.0f, 5.0f);
                    ImGui::SliderFloat("y", &light0_position[1], -5.0f, 5.0f);
                    ImGui::SliderFloat("z", &light0_position[2], -5.0f, 5.0f);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Light1")) {
                    ImGui::ColorEdit4("ambient", light1_ambient);
                    ImGui::ColorEdit4("diffuse", light1_diffuse);
                    ImGui::ColorEdit4("specular", light1_specular);
                    ImGui::Text("position:");
                    ImGui::SliderFloat("x", &light1_position[0], -5.0f, 5.0f);
                    ImGui::SliderFloat("y", &light1_position[1], -5.0f, 5.0f);
                    ImGui::SliderFloat("z", &light1_position[2], -5.0f, 5.0f);
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::End();

            ImGui::SetNextWindowPos(ImVec2(10, io.DisplaySize.y - 10), ImGuiCond_Always, ImVec2(0.0, 1.0));
            ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
            ImGui::Begin("Example: Simple overlay", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
            {
                if (ImGui::IsMousePosValid())
                    ImGui::Text("Mouse Position: %5.1f,%5.1f)", io.MousePos.x, io.MousePos.y);
                else
                    ImGui::Text("Mouse Position: %-13s", "<invalid>");
                ImGui::Text("horizonal angle:%.1f", horizonal_angle);
                ImGui::Text("pitch angle:%.1f", pitch_angle);
                ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
            }
            ImGui::End();
        }


        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 渲染 stanford bunny
        glEnable(GL_VERTEX_ARRAY);
        glEnable(GL_NORMAL_ARRAY);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_RESCALE_NORMAL);
        glEnable(GL_CULL_FACE);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_LIGHT1);

        // 提前加载法向量数组
        glBindBuffer(GL_ARRAY_BUFFER, NBO);
        glNormalPointer(GL_FLOAT, 0, nullptr);

        glPushMatrix();

        glViewport(w - vw, (h - vw) / 2, vw, vw);
        gluPerspective(60, 1, 0.5, 12);

        float cop_x = 2.0f * sin(D2R(pitch_angle));
        float cop_y = 2.0f * cos(D2R(pitch_angle));
        float cop_z = 2.0f * sin(D2R(pitch_angle));
        gluLookAt(
            cop_x, cop_y, cop_z,
            0.0f, 0.0f, 0.0f,
            0.0f, pitch_angle < 180 ? 1.0f : -1.0f, 0.0f
        );

        // 光源设置
        // 0 号光源
        glLightfv (GL_LIGHT0, GL_AMBIENT,  (float*)&light0_ambient);
        glLightfv (GL_LIGHT0, GL_DIFFUSE,  (float*)&light0_diffuse);
        glLightfv (GL_LIGHT0, GL_SPECULAR, (float*)&light0_specular);
        glLightfv (GL_LIGHT0, GL_POSITION, (float*)&light0_position);
        // 1 号光源
        glLightfv (GL_LIGHT1, GL_AMBIENT,  (float*)&light1_ambient);
        glLightfv (GL_LIGHT1, GL_DIFFUSE,  (float*)&light1_diffuse);
        glLightfv (GL_LIGHT1, GL_SPECULAR, (float*)&light1_specular);
        glLightfv (GL_LIGHT1, GL_POSITION, (float*)&light1_position);
        // 全局环境光
        glLightModelfv (GL_LIGHT_MODEL_AMBIENT, (float*)&global_ambient);

        // 材质设置
        glMaterialfv(GL_FRONT, GL_AMBIENT,    (float*)&mat.ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE,    (float*)&mat.diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR,   (float*)&mat.specular);
        //glMaterialfv(GL_FRONT, GL_EMISSION,   (float*)&mat_emission);
        glMaterialf (GL_FRONT, GL_SHININESS,  mat.shininess);

        // 背面剔除
        glCullFace(GL_BACK);

        if (draw_coord) {;
            draw_coordinate();
        }

        // 指出光源位置
        if (draw_lights) {
            glTranslatef(light0_position[0], light0_position[1], light0_position[2]);
            glutSolidSphere(0.05, 10, 10);
            glTranslatef(-light0_position[0], -light0_position[1], -light0_position[2]);
            glTranslatef(light1_position[0], light1_position[1], light1_position[2]);
            glutSolidSphere(0.05, 10, 10);
            glTranslatef(-light1_position[0], -light1_position[1], -light1_position[2]);
        }

        // 水平旋转，注意 Y 轴向上
        glRotatef(horizonal_angle, 0, 1, 0);
        glScalef(0.5f, 0.5f, 0.5f);
        glTranslatef(0.0f, -0.5f, 0.0f);

        if (enable_wire_view) {
            glPolygonMode(GL_FRONT, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT, GL_FILL);
        }

        // 画 Stanford Bunny
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glVertexPointer(3, GL_FLOAT, 0, nullptr);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
        glDrawElements(
            GL_TRIANGLES,
            faces.size(),
            GL_UNSIGNED_INT,
            nullptr
        );

        // 还原状态
        glDisable(GL_RESCALE_NORMAL);
        glDisable(GL_NORMAL_ARRAY);
        glDisable(GL_CULL_FACE);
        glPopMatrix();
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // 渲染 imgui
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void draw_coordinate()
{
    glBegin(GL_LINES);
    {
        glVertex3f(10.0, 0.0, 0.0);
        glVertex3f(-10.0, 0.0, 0.0);
        glVertex3f(10.0, 1.0, 0.0);
        glVertex3f(-10.0, 1.0, 0.0);
        glVertex3f(10.0, 0.0, 1.0);
        glVertex3f(-10.0, 0.0, 1.0);
        glVertex3f(10.0, -1.0, 0.0);
        glVertex3f(-10.0, -1.0, 0.0);
        glVertex3f(10.0, 0.0, -1.0);
        glVertex3f(-10.0, 0.0, -1.0);

        glVertex3f(0.0, 10.0, 0.0);
        glVertex3f(0.0, -10.0, 0.0);
        glVertex3f(1.0, 10.0, 0.0);
        glVertex3f(1.0, -10.0, 0.0);
        glVertex3f(0.0, 10.0, 1.0);
        glVertex3f(0.0, -10.0, 1.0);
        glVertex3f(-1.0, 10.0, 0.0);
        glVertex3f(-1.0, -10.0, 0.0);
        glVertex3f(0.0, 10.0, -1.0);
        glVertex3f(0.0, -10.0, -1.0);

        glVertex3f(0.0, 0.0, 10.0);
        glVertex3f(0.0, 0.0, -10.0);
        glVertex3f(1.0, 0.0, 10.0);
        glVertex3f(1.0, 0.0, -10.0);
        glVertex3f(0.0, 1.0, 10.0);
        glVertex3f(0.0, 1.0, -10.0);
        glVertex3f(-1.0, 0.0, 10.0);
        glVertex3f(-1.0, 0.0, -10.0);
        glVertex3f(0.0, -1.0, 10.0);
        glVertex3f(0.0, -1.0, -10.0);

    }
    glEnd();
}
