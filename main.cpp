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
#include <algorithm>
#include <SDL.h>
#include <SDL_opengl.h>
#include <GL/glu.h>

static void draw_coordinate();
static void set_light_attribute();
static void set_lookat();
static void draw_model();
static void draw_model_select_vertex();
static void draw_model_select_face();

// Degree to Radian
inline float D2R(float degree)
{
    return degree * M_PI / 180.0f;
}

// 模型数据
static std::vector<GLfloat> vertices;
static std::vector<GLuint> faces;
static std::vector<GLfloat> normals;
static GLuint VBO, IBO, NBO;

// Our state
static GLfloat clear_color[4] = {0.34f, 0.82f, 0.82f, 1.00f};         // 清屏颜色
static GLfloat global_ambient[4] = { 0.1, 0.1, 0.1, 0.0 };            // 全局环境光

static GLfloat light0_ambient[4] = {0.1f, 0.1f, 0.1f, 1.0f};          // 光源 0 环境光
static GLfloat light0_diffuse[4] = {1.0f, 1.0f, 1.0f, 1.0f};          // 光源 0 漫反射光
static GLfloat light0_specular[4] = {1.0f, 1.0f, 1.0f, 1.0f};         // 光源 0 镜面反射光

static GLfloat light1_ambient[4] = {0.1f, 0.1f, 0.1f, 1.0f};          // 光源 1 环境光
static GLfloat light1_diffuse[4] = {1.0f, 1.0f, 1.0f, 1.0f};          // 光源 1 漫反射光
static GLfloat light1_specular[4] = {1.0f, 1.0f, 1.0f, 1.0f};         // 光源 1 镜面反射光

static Material mat = materials[0];                                   // 材质参数

static GLfloat light0_position[4] = { 2.3f, 1.0f, 0.23f, 1.0 };       // 光源 0 位置
static GLfloat light1_position[4] = { -2.5f, -0.65f, 1.5f, 1.0f };    // 光源 1 位置

static bool draw_coord = false;                                       // 绘制坐标系辅助线
static bool draw_lights = false;                                      // 绘制光源位置提示球
static bool enable_wire_view = false;                                 // 启用线框模式绘制模型

static float horizonal_angle = 45.0f;                                 // 水平转动角，单位为度
static float pitch_angle = 60.0f;                                     // 俯仰角，与 y 轴正方向夹角，单位为度
static float fovy = 30.0f;                                            // 观察张角

enum {SELECT_NONE = 0, SELECT_VERTEX = 1, SELECT_FACE = 2};
static int select_mode = SELECT_NONE;                                 // 0：不开启点选，1：选择顶点，2：选择面片
static bool lb_clicked = false;                                       // 左键点击：按下，不移动，松开
static ImVec2 lb_press_pos;                                           // 左键按下的位置
static bool select_dispaly = false;                                   // 强调显示被选取的对象
static GLint selected_id;                                             // 被选择的对象在数组中开始位置
static GLdouble select_radius = 1.0f;                                 // 选择视口的半径

struct {
    GLint x, y, w, h;
} static viewport;                                                    // 视口参数

const size_t SELECT_BUF_SIZE = 128;
GLuint select_buffer[SELECT_BUF_SIZE];

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
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Stanford Bunny", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1200, 600, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup GLEW
    //glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "Failed to initalize GLEW: %s\n", glewGetErrorString(err));
        return -1;
    }

    // 加载 Stanford Bunny 数据
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

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();

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

        // 更新视口参数
        viewport.w = std::min({(GLint)io.DisplaySize.x, (GLint)io.DisplaySize.y, (GLint)800});
        viewport.h = viewport.w;
        viewport.x = io.DisplaySize.x - viewport.w;
        viewport.y = (io.DisplaySize.y - viewport.h) / 2;
        glViewport(viewport.x, viewport.y, viewport.w, viewport.h);

        // 更新姿态
        lb_clicked = false;
        if (ImGui::GetMousePos().x > viewport.x) {
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
                } else if (pitch_angle < 0) {
                    pitch_angle += 360;
                }
                ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
            }
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                lb_press_pos = ImGui::GetMousePos();
            } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                auto lb_cur_pos = ImGui::GetMousePos();
                if (lb_cur_pos.x == lb_press_pos.x && lb_cur_pos.y == lb_press_pos.y) {
                    lb_clicked = true;
                }
            }
        }

        // 拾取模式
        GLint hits = 0;
        if (lb_clicked && select_mode != SELECT_NONE) {
            glSelectBuffer(SELECT_BUF_SIZE, select_buffer);
            glRenderMode(GL_SELECT);

            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glLoadIdentity();
            gluPickMatrix(lb_press_pos.x, io.DisplaySize.y - lb_press_pos.y,
                          select_radius * 2, select_radius * 2, (GLint*)&viewport);
            gluPerspective(fovy, 1, 0.1, 20);

            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();
            set_lookat();

            // 绘制模型
            switch (select_mode) {
            case SELECT_VERTEX:
                draw_model_select_vertex();
                break;
            case SELECT_FACE:
                draw_model_select_face();
                break;
            }

            // 恢复先前矩阵
            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();
            glMatrixMode(GL_PROJECTION);
            glPopMatrix();

            // 回到渲染模式并得到选中物体的数目
            hits = glRenderMode(GL_RENDER);
            printf("hits: %d\n", hits);
            if (hits >= 1) {
                GLuint minz = select_buffer[1];
                selected_id = select_buffer[3];
                size_t i = 0;
                for (GLint n = 0; n < hits; ++n) {
                    if (select_buffer[i] == 0) {
                        printf("name: <None>\n");
                        printf("min z: %u\n", select_buffer[i + 1]);
                        printf("max z: %u\n", select_buffer[i + 2]);
                        i += 3;
                    } else {
                        printf("name: ");
                        for (GLuint m = i + 3; m < i + 3 + select_buffer[i]; ++m) {
                            printf("%u ", select_buffer[m]);
                        }
                        printf("\n");
                        printf("min z: %u\n", select_buffer[i + 1]);
                        printf("max z: %u\n", select_buffer[i + 2]);
                        if (select_buffer[i + 1] < minz) {
                            minz = select_buffer[i + 1];
                            selected_id = select_buffer[i + 3];
                        }
                        i += 3 + select_buffer[i];
                    }
                }
                printf("selected id: %d\n", selected_id);
            }

        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        // UI 设计代码
        {
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
            ImGui::Begin("Control");

            if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None)) {
                if (ImGui::BeginTabItem("Global")) {
                    ImGui::ColorEdit3("clear color", clear_color);
                    ImGui::ColorEdit3("global ambient", global_ambient);
                    ImGui::SliderFloat("fovy", &fovy, 1.0f, 90.0f);
                    ImGui::Checkbox("draw coordinate", &draw_coord);
                    ImGui::Checkbox("draw lights", &draw_lights);
                    ImGui::Checkbox("wire view", &enable_wire_view);
                    ImGui::Separator();
                    ImGui::Text("Select Mode");
                    ImGui::RadioButton("None", &select_mode, SELECT_NONE); ImGui::SameLine();
                    ImGui::RadioButton("Vertex", &select_mode, SELECT_VERTEX); ImGui::SameLine();
                    ImGui::RadioButton("Face", &select_mode, SELECT_FACE);
                    {
                        char current_radius[32];
                        static int radius_i = select_radius * 2;
                        sprintf(current_radius, "%.1lf", select_radius);
                        ImGui::SliderInt("Select Radius", &radius_i, 1, 20, current_radius);
                        select_radius = (GLdouble)radius_i / 2;
                    }
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
            ImGui::Begin("overlay", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
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

            if (select_mode != SELECT_NONE && lb_clicked && hits >= 1) {
                ImGui::OpenPopup("#select popup");
            }
            ImVec2 popup_pos(lb_press_pos.x + 10, lb_press_pos.y - 10);
            ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Always, ImVec2(0.0, 1.0));
            select_dispaly = ImGui::BeginPopup("#select popup");
            if (select_dispaly) {
                if (select_mode == SELECT_VERTEX) {
                    ImGui::Text("vertex %d", selected_id / 3);
                    ImGui::Text("(%f, %f, %f)", vertices[selected_id], vertices[selected_id + 1], vertices[selected_id + 2]);
                    ImGui::EndPopup();
                } if (select_mode == SELECT_FACE) {
                    auto v1 = faces[selected_id];
                    auto v2 = faces[selected_id + 1];
                    auto v3 = faces[selected_id + 2];
                    ImGui::Text("triangle %d", selected_id / 3);
                    ImGui::Text("v1: (%f, %f, %f)", vertices[v1], vertices[v1 + 1], vertices[v1 + 2]);
                    ImGui::Text("v2: (%f, %f, %f)", vertices[v2], vertices[v2 + 1], vertices[v2 + 2]);
                    ImGui::Text("v3: (%f, %f, %f)", vertices[v3], vertices[v3 + 1], vertices[v3 + 2]);
                    ImGui::EndPopup();
                }
            }
        }


        // Rendering
        ImGui::Render();
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

        // 背面剔除
        glCullFace(GL_BACK);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluPerspective(fovy, 1, 0.1, 20);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        set_lookat();

        // 光源及材质设置
        set_light_attribute();

        if (draw_coord) {
            draw_coordinate();
        }

        // 绘制模型
        draw_model();

        // 指出光源位置
        if (draw_lights) {
            glPolygonMode(GL_FRONT, GL_FILL);
            glLoadIdentity();
            set_lookat();
            glTranslatef(light0_position[0], light0_position[1], light0_position[2]);
            drawSolidSphere(0.05, 16, 16);
            glLoadIdentity();
            set_lookat();
            glTranslatef(light1_position[0], light1_position[1], light1_position[2]);
            drawSolidSphere(0.05, 16, 16);
        }

        // 强调被选中的顶点
        if (select_dispaly && select_mode == SELECT_VERTEX) {
            glDisable(GL_LIGHTING);
            glPolygonMode(GL_FRONT, GL_FILL);
            glColor3i(0, 0, 0);
            glLoadIdentity();
            set_lookat();
            // 水平旋转，注意 Y 轴向上
            glRotatef(horizonal_angle, 0, 1, 0);
            //glScalef(0.5f, 0.5f, 0.5f);
            glTranslatef(0.0f, -0.5f, 0.0f);
            glTranslatef(vertices[selected_id], vertices[selected_id + 1], vertices[selected_id + 2]);
            drawSolidSphere(0.01, 10, 10);
        }

        // 强调被点选的面片
        if (select_dispaly && select_mode == SELECT_FACE) {
            glDisable(GL_LIGHTING);
            glPolygonMode(GL_FRONT, GL_FILL);
            glColor3i(0, 0, 0);
            glLoadIdentity();
            set_lookat();
            // 水平旋转，注意 Y 轴向上
            glRotatef(horizonal_angle, 0, 1, 0);
            //glScalef(0.5f, 0.5f, 0.5f);
            glTranslatef(0.0f, -0.5f, 0.0f);
            glBegin(GL_TRIANGLES);
            glVertex3fv(vertices.data() + faces[selected_id] * 3);
            glVertex3fv(vertices.data() + faces[selected_id + 1] * 3);
            glVertex3fv(vertices.data() + faces[selected_id + 2] * 3);
            glEnd();
        }

        // 还原状态
        glDisable(GL_RESCALE_NORMAL);
        glDisable(GL_NORMAL_ARRAY);
        glDisable(GL_CULL_FACE);
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
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

static void set_lookat()
{
    // 注意y轴朝上
    float cop_x = 10.0f * 0.707f * sin(D2R(pitch_angle));
    float cop_y = 10.0f * cos(D2R(pitch_angle));
    float cop_z = 10.0f * 0.707f * sin(D2R(pitch_angle));
    gluLookAt(
        cop_x, cop_y, cop_z,
        0.0f, 0.0f, 0.0f,
        0.0f, pitch_angle < 180 ? 1.0f : -1.0f, 0.0f
    );
}

static void draw_coordinate()
{
    const static GLfloat coord_lines[][3] = {
        {10.0, 0.0, 0.0},
        { -10.0, 0.0, 0.0},
        {10.0, 1.0, 0.0},
        { -10.0, 1.0, 0.0},
        {10.0, 0.0, 1.0},
        { -10.0, 0.0, 1.0},
        {10.0, -1.0, 0.0},
        { -10.0, -1.0, 0.0},
        {10.0, 0.0, -1.0},
        { -10.0, 0.0, -1.0},
        {0.0, 10.0, 0.0},
        {0.0, -10.0, 0.0},
        {1.0, 10.0, 0.0},
        {1.0, -10.0, 0.0},
        {0.0, 10.0, 1.0},
        {0.0, -10.0, 1.0},
        { -1.0, 10.0, 0.0},
        { -1.0, -10.0, 0.0},
        {0.0, 10.0, -1.0},
        {0.0, -10.0, -1.0},
        {0.0, 0.0, 10.0},
        {0.0, 0.0, -10.0},
        {1.0, 0.0, 10.0},
        {1.0, 0.0, -10.0},
        {0.0, 1.0, 10.0},
        {0.0, 1.0, -10.0},
        { -1.0, 0.0, 10.0},
        { -1.0, 0.0, -10.0},
        {0.0, -1.0, 10.0},
        {0.0, -1.0, -10.0},
    };
    glBindBuffer(GL_VERTEX_ARRAY, 0);
    glVertexPointer(3, GL_FLOAT, 0, coord_lines);
    glDrawArrays(GL_LINES, 0, sizeof(coord_lines) / sizeof(GLfloat) / 3);
}

// 光源及材质设置
static void set_light_attribute()
{
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
}

static void draw_model()
{
    // 水平旋转，注意 Y 轴向上
    glRotatef(horizonal_angle, 0, 1, 0);
    //glScalef(0.5f, 0.5f, 0.5f);
    glTranslatef(0.0f, -0.5f, 0.0f);

    if (enable_wire_view) {
        glPolygonMode(GL_FRONT, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT, GL_FILL);
    }

    // 画 Stanford Bunny
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexPointer(3, GL_FLOAT, 0, nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, NBO);
    glNormalPointer(GL_FLOAT, 0, nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glDrawElements(
        GL_TRIANGLES,
        faces.size(),
        GL_UNSIGNED_INT,
        nullptr
    );
}

static void draw_model_select_vertex()
{
    // 水平旋转，注意 Y 轴向上
    glRotatef(horizonal_angle, 0, 1, 0);
    //glScalef(0.5f, 0.5f, 0.5f);
    glTranslatef(0.0f, -0.5f, 0.0f);

    glInitNames();
    glPushName(-1);
    auto vd = vertices.data();
    for (size_t v = 0; v < vertices.size(); v += 3) {
        glLoadName(v);
        glBegin(GL_POINTS);
        glVertex3fv(vd + v);
        glEnd();
    }
}

static void draw_model_select_face()
{
    // 水平旋转，注意 Y 轴向上
    glRotatef(horizonal_angle, 0, 1, 0);
    //glScalef(0.5f, 0.5f, 0.5f);
    glTranslatef(0.0f, -0.5f, 0.0f);

    glInitNames();
    glPushName(-1);
    auto vd = vertices.data();
    auto fd = faces.data();
    for (size_t f = 0; f < faces.size(); f += 3) {
        glLoadName(f);
        glBegin(GL_TRIANGLES);
        glVertex3fv(vd + fd[f] * 3);
        glVertex3fv(vd + fd[f + 1] * 3);
        glVertex3fv(vd + fd[f + 2] * 3);
        glEnd();
    }
}
