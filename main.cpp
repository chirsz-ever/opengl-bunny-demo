#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"

#include "materials.h"
#include "utils.h"

static void glfw_error_callback(int error, const char *description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static void print_opengl_info() {
    printf("OpenGL version: %s\n", glGetString(GL_VERSION));
}

static void print_glfw_version() {
    int major, minor, rev;
    glfwGetVersion(&major, &minor, &rev);

    printf("GLFW version:\n");
    printf("\tcompile-time: %s\n", glfwGetVersionString());
    printf("\t    run-time: %d.%d.%d\n", major, minor, rev);
}

static void print_glew_version() {
    printf("GLEW version:\n");
    printf("\tcompile-time: %d.%d.%d\n", GLEW_VERSION_MAJOR, GLEW_VERSION_MINOR, GLEW_VERSION_MICRO);
    printf("\t    run-time: %s\n", glewGetString(GLEW_VERSION));
}

// Degree to Radian
inline float D2R(float degree) { return glm::radians(degree); }

inline void glLoadTopMatrix() {
    glPopMatrix();
    glPushMatrix();
}

class Application {
public:
    Application(int argc, const char *const *argv) : argc(argc), argv(argv) {
        (void)argc;
        (void)argv;
    }

    int run() {
        loadModel();
        initWindow();
        initOpenGL();
        initImgui();
        mainLoop();
        cleanup();
        return 0;
    }

private:
    const int argc;
    const char *const *const argv;

    GLFWwindow *window = nullptr;

    // 模型数据
    std::vector<GLfloat> vertices;
    std::vector<GLuint> faces;
    std::vector<GLfloat> normals;

    GLuint VBO, IBO, NBO;
    
    GLuint program_phong, program_simple;

    // Our state
    GLfloat clear_color[4] = {0.34f, 0.82f, 0.82f, 1.00f}; // 清屏颜色
    GLfloat global_ambient[4] = {0.1, 0.1, 0.1, 0.0};      // 全局环境光

    GLfloat light0_ambient[4] = {0.1f, 0.1f, 0.1f, 1.0f};  // 光源 0 环境光
    GLfloat light0_diffuse[4] = {1.0f, 1.0f, 1.0f, 1.0f};  // 光源 0 漫反射光
    GLfloat light0_specular[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // 光源 0 镜面反射光

    GLfloat light1_ambient[4] = {0.1f, 0.1f, 0.1f, 1.0f};  // 光源 1 环境光
    GLfloat light1_diffuse[4] = {1.0f, 1.0f, 1.0f, 1.0f};  // 光源 1 漫反射光
    GLfloat light1_specular[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // 光源 1 镜面反射光

    Material mat = materials[0]; // 材质参数

    GLfloat light0_position[4] = {2.3f, 1.0f, 0.23f, 1.0};    // 光源 0 位置
    GLfloat light1_position[4] = {-2.5f, -0.65f, 1.5f, 1.0f}; // 光源 1 位置

    GLfloat wire_color[4] = {0.1, 0.1, 0.1, 1.0};

    bool draw_coord = false;       // 绘制坐标系辅助线
    bool draw_lights = false;      // 绘制光源位置提示球
    bool enable_wire_view = false; // 启用线框模式绘制模型
    bool show_back_wire = false;   // 显示模型另一侧的线框

    float horizonal_angle = 45.0f; // 水平转动角，单位为度
    float pitch_angle = 60.0f;     // 俯仰角，与 y 轴正方向夹角，单位为度
    float fovy = 30.0f;            // 观察张角
    float view_distance = 10.0f;   // 观察距离

    enum { SELECT_NONE = 0, SELECT_VERTEX = 1, SELECT_FACE = 2 };
    int select_mode = SELECT_NONE; // 0：不开启点选，1：选择顶点，2：选择面片
    bool lb_clicked = false;       // 左键点击：按下，不移动，松开
    ImVec2 lb_press_pos;           // 左键按下的位置
    bool select_dispaly = false;   // 强调显示被选取的对象
    GLint selected_id;             // 被选择的对象在数组中开始位置
    GLdouble select_radius = 1.0f; // 选择视口的半径

    // 视口参数
    struct {
        GLint x, y, w, h;
    } viewport;

    bool inViewPort(const ImVec2 &pos) {
        return pos.x >= viewport.x && pos.y >= viewport.y && pos.x < (pos.x + viewport.w) &&
               pos.y < (viewport.y + viewport.h);
    }

    constexpr static size_t SELECT_BUF_SIZE = 128;
    GLuint select_buffer[SELECT_BUF_SIZE];

    void initWindow() {
        // Setup window
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
            throw std::runtime_error("glfw init failed");

        print_glfw_version();

        glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
        glfwWindowHint(GLFW_SAMPLES, 4);
        window = glfwCreateWindow(1200, 600, "Stanford Bunny", NULL, NULL);
        if (window == NULL)
            throw std::runtime_error("glfw create window failed");

        const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glfwSetWindowPos(window, (mode->width - 1200) / 2, (mode->height - 600) / 2);
        glfwShowWindow(window);
    }

    void initOpenGL() {
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable vsync

        print_opengl_info();

        // Setup GLEW
        // glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            std::string glewErrorString = (const char*)glewGetErrorString(err);
            throw std::runtime_error("glew init failed: " + glewErrorString);
        }

        print_glew_version();

        program_phong = load_program("shaders/phong.vert", "shaders/phong.frag");
        glBindAttribLocation(program_phong, 0, "position");
        glBindAttribLocation(program_phong, 1, "normal");
        glLinkProgram(program_phong);

        program_simple = load_program("shaders/simple.vert", "shaders/simple.frag");
        glBindAttribLocation(program_simple, 0, "position");
        glBindAttribLocation(program_simple, 2, "color");
        glLinkProgram(program_simple);

        // 顶点缓冲区对象
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // 顶点索引缓冲区对象
        glGenBuffers(1, &IBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(GLuint), faces.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // 法向量顶点缓冲区对象
        glGenBuffers(1, &NBO);
        glBindBuffer(GL_ARRAY_BUFFER, NBO);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat), normals.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

    }

    void loadModel() {
        // 加载 Stanford Bunny 数据
        const char *filename = "bunny.obj";
        if (argc >= 2) {
            filename = argv[1];
        }
        load_bunny_data(filename, vertices, faces, normals);

        printf("%s loaded, vertices:%lu, faces:%lu, normals:%lu\n", filename, (unsigned long)vertices.size() / 3,
            (unsigned long)faces.size() / 3, (unsigned long)normals.size() / 3);
    }

    void initImgui() {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        // ImGui::StyleColorsClassic();

        // Setup Platform/Renderer bindings
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL2_Init();
    }

    void cleanup() {
        ImGui_ImplOpenGL2_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    // 渲染模型
    // TODO: 完全使用自定义 shader 变量传递定点属性和变换矩阵
    void draw_model() {
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glUseProgram(program_phong);
        model_transform();

        // 顶点坐标
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        // 法向量
        glBindBuffer(GL_ARRAY_BUFFER, NBO);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        // 顶点索引
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

        glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, nullptr);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    // 绘制模型线框
    void draw_wire_model() {
        glEnableVertexAttribArray(0);
        // 需要禁用索引为 2 的顶点属性数组，否则绘制函数会认为
        // 定点属性数据被 glVertexAttribPointer 指定，而 glVertexAttrib 无用
        glDisableVertexAttribArray(2);
        glPolygonMode(GL_FRONT, GL_LINE);
        if (show_back_wire) {
            glDisable(GL_CULL_FACE);
            glPolygonMode(GL_BACK, GL_LINE);
        }
        glUseProgram(program_simple);
        model_transform();

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        glVertexAttrib4fv(2, wire_color);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

        glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, nullptr);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDisableVertexAttribArray(0);
        glPolygonMode(GL_FRONT, GL_FILL);
        if (show_back_wire) {
            glEnable(GL_CULL_FACE);
            glPolygonMode(GL_BACK, GL_FILL);
        }
    }

    // 在光源位置绘制小球
    // TODO: 绘制光球效果
    void draw_light_balls() {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_INDEX_ARRAY);
        glUseProgram(0);

        glLoadTopMatrix();
        glTranslatef(light0_position[0], light0_position[1], light0_position[2]);
        glColor3fv(light0_diffuse);
        drawSolidSphere(0.05, 16, 16);

        glLoadTopMatrix();
        glTranslatef(light1_position[0], light1_position[1], light1_position[2]);
        glColor3fv(light1_diffuse);
        drawSolidSphere(0.05, 16, 16);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_INDEX_ARRAY);
    }

    // 强调被选中的顶点
    // TODO: 在 shader 中使用 gl_PointSize 和 gl_PointCoord 绘制圆点    
    void draw_selected_vertex() {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_INDEX_ARRAY);
        glUseProgram(0);
        glDisable(GL_LIGHTING);
        glPolygonMode(GL_FRONT, GL_FILL);
        glColor3i(0, 0, 0);

        glLoadTopMatrix();
        model_transform();
        glTranslatef(vertices[selected_id], vertices[selected_id + 1], vertices[selected_id + 2]);
        drawSolidSphere(0.01, 10, 10);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_INDEX_ARRAY);
    }

    // 绘制被点选的面片
    void draw_selected_face() {
        glEnableVertexAttribArray(0);
        glDisableVertexAttribArray(2);
        glUseProgram(program_simple);

        glLoadTopMatrix();
        model_transform();

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        glVertexAttrib3f(2, 0.f, 0.f, 0.f);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

        // 使用 IBO 时，最后参数表示 IBO 中以字节为单位的偏移
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, reinterpret_cast<const void*>(selected_id * sizeof(GLuint)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDisableVertexAttribArray(0);
    }

    // clang-format off
// Main code
void mainLoop() {
    ImGuiIO &io = ImGui::GetIO();
    while (!glfwWindowShouldClose(window)) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your
        // inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those
        // two flags.
        glfwPollEvents();

        // ImGUI preparation for the frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        // 更新视口参数
        viewport.w = std::min({(GLint)io.DisplaySize.x, (GLint)io.DisplaySize.y, (GLint)800});
        viewport.h = viewport.w;
        viewport.x = io.DisplaySize.x - viewport.w;
        viewport.y = (io.DisplaySize.y - viewport.h) / 2;
        // 在 rentia 这样的屏幕上上需要如此适配缩放
        {
            float x = viewport.x * io.DisplayFramebufferScale.x;
            float y = viewport.y * io.DisplayFramebufferScale.y;
            float w = viewport.w * io.DisplayFramebufferScale.x;
            float h = viewport.h * io.DisplayFramebufferScale.y;
            glViewport(x, y, w, h);
        }
        // 更新姿态
        lb_clicked = false;

        const auto mouse_pos = ImGui::GetMousePos();
        if (inViewPort(mouse_pos)) {
            // 偏航角
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                auto dd = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
                ImVec2 clicked_pos(mouse_pos.x - dd.x, mouse_pos.y - dd.y);
                if (inViewPort(clicked_pos)) {
                    horizonal_angle += io.MouseDelta.x;
                }
            }

            // 俯仰角
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
                auto dd = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
                ImVec2 clicked_pos(mouse_pos.x - dd.x, mouse_pos.y - dd.y);
                if (inViewPort(clicked_pos)) {
                    pitch_angle -= io.MouseDelta.y;
                }
                if (pitch_angle >= 360) {
                    pitch_angle -= 360;
                } else if (pitch_angle < 0) {
                    pitch_angle += 360;
                }
            }

            // 点选
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                lb_press_pos = mouse_pos;
            } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                if (mouse_pos.x == lb_press_pos.x && mouse_pos.y == lb_press_pos.y) {
                    lb_clicked = true;
                }
            }

            // 滚轮
            view_distance -= io.MouseWheel * 0.5f;
        }

        // 拾取模式
        // TODO: 使用软件实现
        GLint hits = 0;
        if (lb_clicked && select_mode != SELECT_NONE) {
            glSelectBuffer(SELECT_BUF_SIZE, select_buffer);
            glRenderMode(GL_SELECT);

            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            // glLoadIdentity();
            // gluPickMatrix(lb_press_pos.x, io.DisplaySize.y - lb_press_pos.y, select_radius * 2, select_radius * 2,
            //               (GLint *)&viewport);
            // gluPerspective(fovy, 1, 0.1, 20);

            glm::mat4 proj = glm::pickMatrix(glm::vec2(lb_press_pos.x, io.DisplaySize.y - lb_press_pos.y),
                                             glm::vec2(select_radius * 2, select_radius * 2),
                                             glm::vec4(viewport.x, viewport.y, viewport.w, viewport.h));
            proj *= glm::perspective(glm::radians(fovy), 1.0f, 0.1f, 20.0f);
            glLoadMatrixf(glm::value_ptr(proj));

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
                    ImGui::SliderFloat("fovy", &fovy, 0.1f, 90.0f);
                    ImGui::Checkbox("draw coordinate", &draw_coord);
                    ImGui::Checkbox("draw lights", &draw_lights);
                    ImGui::Checkbox("wire view", &enable_wire_view);
                    if (enable_wire_view) {
                        ImGui::TreePush();
                        ImGui::Checkbox("show back wire", &show_back_wire);
                        ImGui::ColorEdit3("wire color", wire_color);
                        ImGui::TreePop();
                    }
                    ImGui::Separator();
                    ImGui::Text("Select Mode");
                    ImGui::RadioButton("None", &select_mode, SELECT_NONE);
                    ImGui::SameLine();
                    ImGui::RadioButton("Vertex", &select_mode, SELECT_VERTEX);
                    ImGui::SameLine();
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
                    for (const Material &m : materials) {
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
            ImGui::Begin("overlay", nullptr,
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav);
            {
                if (ImGui::IsMousePosValid())
                    ImGui::Text("Mouse Position: (%6.1f,%6.1f)", io.MousePos.x, io.MousePos.y);
                else
                    ImGui::Text("Mouse Position: %-13s", "<invalid>");
                ImGui::Text("mouse left button dragging: %d", ImGui::IsMouseDragging(ImGuiMouseButton_Left));
                ImGui::Text("display size: %6.1f %6.1f, scale: %.1f %.1f", io.DisplaySize.x, io.DisplaySize.y,
                            io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
                ImGui::Text("viewport: %d %d %d %d", viewport.x, viewport.y, viewport.w, viewport.h);
                ImGui::Separator();
                ImGui::Text("view distance: %.2f", view_distance);
                ImGui::Text("horizonal angle:%.1f", horizonal_angle);
                ImGui::Text("pitch angle:%.1f", pitch_angle);
                ImGui::Separator();
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
                    ImGui::Text("(%f, %f, %f)", vertices[selected_id], vertices[selected_id + 1],
                                vertices[selected_id + 2]);
                    ImGui::EndPopup();
                }
                if (select_mode == SELECT_FACE) {
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
        // int display_w, display_h;
        // glfwGetFramebufferSize(window, &display_w, &display_h);
        // glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 三维物体渲染
        // 共用摄像机位姿、投影矩阵、深度缓存
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        // glLoadIdentity();
        // gluPerspective(fovy, 1, 0.1, 20);
        glm::mat4 proj = glm::perspective(glm::radians(fovy), 1.0f, 0.1f, 1000.0f);
        glLoadMatrixf(glm::value_ptr(proj));

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        set_lookat();

        // 光源及材质设置
        // 光源位置会受模视矩阵影响，必须和其它步骤处于同一世界坐标系下
        set_light_attribute();

        // 保存视图矩阵
        glPushMatrix();

        if (draw_coord) {
            draw_coordinate();
        }

        // 绘制模型或线框
        if (enable_wire_view) {
            draw_wire_model();
        } else {
            draw_model();
        }

        // 指出光源位置
        if (draw_lights) {
            draw_light_balls();
        }

        // 强调被选中的顶点
        if (select_dispaly && select_mode == SELECT_VERTEX) {
            draw_selected_vertex();
        }

        // 强调被点选的面片
        if (select_dispaly && select_mode == SELECT_FACE) {
            draw_selected_face();
        }

        // 还原状态
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // 渲染 imgui
        glUseProgram(0);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
}

void set_lookat() {
    // 注意y轴朝上
    float cop_x = view_distance * sqrt(0.5f) * sin(D2R(pitch_angle));
    float cop_y = view_distance * cos(D2R(pitch_angle));
    float cop_z = view_distance * sqrt(0.5f) * sin(D2R(pitch_angle));

    float up[3] = {0.0f, 0.0f, 0.0f};
    if (pitch_angle == 0) {
        up[0] = up[2] = -1.0f;
    } else if (pitch_angle == 180) {
        up[0] = up[2] = 1.0f;
    } else if (pitch_angle < 180) {
        up[1] = 1.0f;
    } else {
        up[1] = -1.0f;
    }
    // gluLookAt(cop_x, cop_y, cop_z, 0.0f, 0.0f, 0.0f, up[0], up[1], up[2]);
    glm::mat4 lookat =
        glm::lookAt(glm::vec3(cop_x, cop_y, cop_z), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(up[0], up[1], up[2]));
    glMultMatrixf(glm::value_ptr(lookat));
}

void model_transform() {
    // 水平旋转，注意 Y 轴向上
    glRotatef(horizonal_angle, 0, 1, 0);
    // glScalef(0.5f, 0.5f, 0.5f);
    glTranslatef(0.0f, -0.5f, 0.0f);
}

void draw_coordinate() {
    glEnableClientState(GL_VERTEX_ARRAY);
    glUseProgram(0);
    const static GLfloat coord_lines[][3] = {
        {10.0, 0.0, 0.0},  {-10.0, 0.0, 0.0}, {10.0, 1.0, 0.0},   {-10.0, 1.0, 0.0}, {10.0, 0.0, 1.0},
        {-10.0, 0.0, 1.0}, {10.0, -1.0, 0.0}, {-10.0, -1.0, 0.0}, {10.0, 0.0, -1.0}, {-10.0, 0.0, -1.0},
        {0.0, 10.0, 0.0},  {0.0, -10.0, 0.0}, {1.0, 10.0, 0.0},   {1.0, -10.0, 0.0}, {0.0, 10.0, 1.0},
        {0.0, -10.0, 1.0}, {-1.0, 10.0, 0.0}, {-1.0, -10.0, 0.0}, {0.0, 10.0, -1.0}, {0.0, -10.0, -1.0},
        {0.0, 0.0, 10.0},  {0.0, 0.0, -10.0}, {1.0, 0.0, 10.0},   {1.0, 0.0, -10.0}, {0.0, 1.0, 10.0},
        {0.0, 1.0, -10.0}, {-1.0, 0.0, 10.0}, {-1.0, 0.0, -10.0}, {0.0, -1.0, 10.0}, {0.0, -1.0, -10.0},
    };
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexPointer(3, GL_FLOAT, 0, coord_lines);
    glColor3f(0.f, 0.f, 0.f);
    glDrawArrays(GL_LINES, 0, sizeof(coord_lines) / sizeof(GLfloat) / 3);
    glDisableClientState(GL_VERTEX_ARRAY);
}

// 光源及材质设置
void set_light_attribute() {
    // 0 号光源
    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
    // 1 号光源
    glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, light1_specular);
    glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
    // 全局环境光
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

    // 材质设置
    glMaterialfv(GL_FRONT, GL_AMBIENT, mat.ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat.diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat.specular);
    // glMaterialfv(GL_FRONT, GL_EMISSION,   mat_emission);
    glMaterialf(GL_FRONT, GL_SHININESS, mat.shininess);
}

void draw_model_select_vertex() {
    model_transform();

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

void draw_model_select_face() {
    model_transform();

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
};

// clang-format on

int main(int argc, char **argv) {
    Application app(argc, argv);

    return app.run();
}
