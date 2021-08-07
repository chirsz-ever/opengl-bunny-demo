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
#include "imgui_impl_opengl3.h"

#include "materials.h"
#include "utils.h"

static void glfw_error_callback(int error, const char *description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static void print_opengl_info() {
    printf("OpenGL information:\n");
    printf("\t  version: %s\n", glGetString(GL_VERSION));
    printf("\t   vendor: %s\n", glGetString(GL_VENDOR));
    printf("\t renderer: %s\n", glGetString(GL_RENDERER));
    printf("\t     GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
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

struct LightSource {
    GLfloat ambient[4];  // 环境光
    GLfloat diffuse[4];  // 漫反射
    GLfloat specular[4]; // 镜面反射
    GLfloat position[4]; // 位置
};

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

    constexpr static size_t LIGHTS = 2;

    GLFWwindow *window = nullptr;

    // 模型数据
    Mesh<> model;

    // 缓冲区
    GLuint VBO, IBO, NBO;

    // 程序对象
    GLuint program_phong, program_simple;

    // 模型矩阵
    glm::mat4 mat_model;
    // 观察矩阵
    glm::mat4 mat_view;
    // 投影矩阵
    glm::mat4 mat_proj;

    // Our state
    GLfloat clear_color[4]    = {0.34f, 0.82f, 0.82f, 1.00f}; // 清屏颜色
    GLfloat global_ambient[4] = {0.1, 0.1, 0.1, 0.0};         // 全局环境光

    // 材质参数
    Material material = materials[0];

    // 光源参数
    LightSource lights[LIGHTS] = {
        {
            {0.1f, 0.1f, 0.1f, 1.0f},
            {1.0f, 1.0f, 1.0f, 1.0f},
            {1.0f, 1.0f, 1.0f, 1.0f},
            {2.3f, 1.0f, 0.23f, 1.0f},
        },
        {
            {0.1f, 0.1f, 0.1f, 1.0f},
            {1.0f, 1.0f, 1.0f, 1.0f},
            {1.0f, 1.0f, 1.0f, 1.0f},
            {-2.5f, -0.65f, 1.5f, 1.0f},
        },
    };

    // uniform 位置
    struct {
        struct {
            GLint model;
            GLint view;
            GLint proj;
        } simple;
        struct {
            struct {
                GLint ambient;
                GLint diffuse;
                GLint specular;
                GLint position;
            } lights[LIGHTS];
            struct {
                GLint ambient;
                GLint diffuse;
                GLint specular;
                GLint shininess;
            } material;
            GLint global_ambient;
            GLint model;
            GLint view;
            GLint proj;
        } phong;
    } uniform_locations;

#define GET_UNIFORM_LOCATION(p, u)     uniform_locations.p.u = glGetUniformLocation(program_##p, #u)
#define SET_UNIFORM_N(p, n, u)         glUniform##n##fv(uniform_locations.p.u, 1, u)
#define SET_UNIFORM_1(p, u)            glUniform1f(uniform_locations.p.u, u)
#define SET_UNIFORM_MAT_N(p, n, u)     glUniformMatrix##n##fv(uniform_locations.p.u, 1, GL_FALSE, glm::value_ptr(mat_##u))
//
#define GET_PHONG_UNIFORM_LOCATION(u)  GET_UNIFORM_LOCATION(phong, u)
#define GET_SIMPLE_UNIFORM_LOCATION(u) GET_UNIFORM_LOCATION(simple, u)
//
#define SET_PHONG_UNIFORM4(u)          SET_UNIFORM_N(phong, 4, u)
#define SET_PHONG_UNIFORM1(u)          SET_UNIFORM_1(phong, u)
#define SET_PHONG_UNIFORM_MAT4(u)      SET_UNIFORM_MAT_N(phong, 4, u)
//
#define SET_SIMPLE_UNIFORM_MAT4(u)     SET_UNIFORM_MAT_N(simple, 4, u)

    // 线框颜色
    GLfloat wire_color[4] = {0.1, 0.1, 0.1, 1.0};

    bool draw_coord       = false; // 绘制坐标系辅助线
    bool draw_lights      = false; // 绘制光源位置提示球
    bool enable_wire_view = false; // 启用线框模式绘制模型
    bool show_back_wire   = false; // 显示模型另一侧的线框

    float horizonal_angle = 45.0f; // 水平转动角，单位为度
    float pitch_angle     = 60.0f; // 俯仰角，与 y 轴正方向夹角，单位为度
    float fovy            = 30.0f; // 观察张角
    float view_distance   = 10.0f; // 观察距离

    enum { SELECT_NONE = 0, SELECT_VERTEX = 1, SELECT_FACE = 2 };
    int select_mode = SELECT_NONE;  // 0：不开启点选，1：选择顶点，2：选择面片
    bool lb_clicked = false;        // 左键点击：按下，不移动，松开
    ImVec2 lb_press_pos;            // 左键按下的位置
    bool select_dispaly = false;    // 强调显示被选取的对象
    GLint selected_id;              // 被选择的对象在数组中开始位置
    GLdouble select_radius = 1.0f;  // 选择视口的半径
    bool pick_sucess       = false; // 在本帧中进行拾取且成功

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
            std::string glewErrorString = (const char *)glewGetErrorString(err);
            throw std::runtime_error("glew init failed: " + glewErrorString);
        }

        print_glew_version();

        // Phong 光照模型
        program_phong = load_program("shaders/phong.vert", "shaders/phong.frag");
        glBindAttribLocation(program_phong, 0, "position");
        glBindAttribLocation(program_phong, 1, "normal");
        glLinkProgram(program_phong);
        get_phong_uniform_locations();

        // 简单着色器
        program_simple = load_program("shaders/simple.vert", "shaders/simple.frag");
        glBindAttribLocation(program_simple, 0, "position");
        glBindAttribLocation(program_simple, 2, "color");
        glLinkProgram(program_simple);
        get_simple_uniform_locations();

        GLuint buffers[] = {VBO, IBO, NBO};
        glGenBuffers(std::end(buffers) - std::begin(buffers), buffers);
        VBO = buffers[0];
        IBO = buffers[1];
        NBO = buffers[2];

        // 顶点缓冲区对象
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, model.vertices.size() * sizeof(GLfloat), model.vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // 顶点索引缓冲区对象
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size() * sizeof(GLuint), model.indices.data(),
                     GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // 法向量顶点缓冲区对象
        glBindBuffer(GL_ARRAY_BUFFER, NBO);
        glBufferData(GL_ARRAY_BUFFER, model.normals.size() * sizeof(GLfloat), model.normals.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void get_phong_uniform_locations() {
        GET_PHONG_UNIFORM_LOCATION(lights[0].ambient);
        GET_PHONG_UNIFORM_LOCATION(lights[0].diffuse);
        GET_PHONG_UNIFORM_LOCATION(lights[0].specular);
        GET_PHONG_UNIFORM_LOCATION(lights[0].position);

        GET_PHONG_UNIFORM_LOCATION(lights[1].ambient);
        GET_PHONG_UNIFORM_LOCATION(lights[1].diffuse);
        GET_PHONG_UNIFORM_LOCATION(lights[1].specular);
        GET_PHONG_UNIFORM_LOCATION(lights[1].position);

        GET_PHONG_UNIFORM_LOCATION(material.ambient);
        GET_PHONG_UNIFORM_LOCATION(material.diffuse);
        GET_PHONG_UNIFORM_LOCATION(material.specular);
        GET_PHONG_UNIFORM_LOCATION(material.shininess);

        GET_PHONG_UNIFORM_LOCATION(global_ambient);

        GET_PHONG_UNIFORM_LOCATION(model);
        GET_PHONG_UNIFORM_LOCATION(view);
        GET_PHONG_UNIFORM_LOCATION(proj);
    }

    void get_simple_uniform_locations() {
        GET_SIMPLE_UNIFORM_LOCATION(model);
        GET_SIMPLE_UNIFORM_LOCATION(view);
        GET_SIMPLE_UNIFORM_LOCATION(proj);
    }

    void loadModel() {
        // 加载 Stanford Bunny 数据
        const char *filename = "bunny.obj";
        if (argc >= 2) {
            filename = argv[1];
        }
        model = load_bunny_data(filename);

        printf("%s loaded, vertices:%lu, faces:%lu, normals:%lu\n", filename, (unsigned long)model.vertices.size() / 3,
               (unsigned long)model.indices.size() / 3, (unsigned long)model.normals.size() / 3);
    }

    // 设置模型姿态
    void set_model_transform() {
        // 水平旋转，注意 Y 轴向上
        mat_model = glm::rotate(glm::identity<glm::mat4>(), glm::radians(horizonal_angle), glm::vec3{0.0f, 1.0f, 0.0f});
        mat_model = glm::translate(mat_model, glm::vec3{0.0f, -0.5f, 0.0f});
    }

    void model_transform() {
        glMultMatrixf(glm::value_ptr(mat_model));
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
        ImGui_ImplOpenGL3_Init();
    }

    void cleanup() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        const GLuint buffers[] = {VBO, IBO, NBO};
        glDeleteBuffers(std::end(buffers) - std::begin(buffers), buffers);

        // 清除程序对象和 shader 对象
        glUseProgram(0);
        cleanup_program(program_simple);
        cleanup_program(program_phong);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    static void cleanup_program(GLuint program) {
        GLuint shaders[2];
        glGetAttachedShaders(program, 2, nullptr, shaders);
        glDeleteProgram(program);
        glDeleteShader(shaders[0]);
        glDeleteShader(shaders[1]);
    }

    // 光源及材质设置
    void set_phong_uniform() {
        glUseProgram(program_phong);
        // 0 号光源
        SET_PHONG_UNIFORM4(lights[0].ambient);
        SET_PHONG_UNIFORM4(lights[0].diffuse);
        SET_PHONG_UNIFORM4(lights[0].specular);
        SET_PHONG_UNIFORM4(lights[0].position);
        // 1 号光源
        SET_PHONG_UNIFORM4(lights[1].ambient);
        SET_PHONG_UNIFORM4(lights[1].diffuse);
        SET_PHONG_UNIFORM4(lights[1].specular);
        SET_PHONG_UNIFORM4(lights[1].position);
        // 全局环境光
        SET_PHONG_UNIFORM4(global_ambient);

        // 材质设置
        SET_PHONG_UNIFORM4(material.ambient);
        SET_PHONG_UNIFORM4(material.diffuse);
        SET_PHONG_UNIFORM4(material.specular);
        SET_PHONG_UNIFORM1(material.shininess);

        SET_PHONG_UNIFORM_MAT4(model);
        SET_PHONG_UNIFORM_MAT4(view);
        SET_PHONG_UNIFORM_MAT4(proj);
    }

    void set_simple_uniform() {
        glUseProgram(program_simple);
        SET_SIMPLE_UNIFORM_MAT4(view);
        SET_SIMPLE_UNIFORM_MAT4(proj);
    }

    // 渲染模型
    void draw_model() {
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glUseProgram(program_phong);

        // 顶点坐标
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        // 法向量
        glBindBuffer(GL_ARRAY_BUFFER, NBO);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        // 顶点索引
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

        glDrawElements(GL_TRIANGLES, model.indices.size(), GL_UNSIGNED_INT, nullptr);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    // 画坐标轴
    // TODO: 使用顶点缓存
    void draw_coordinate() {
        glEnableVertexAttribArray(0);
        glDisableVertexAttribArray(2);
        glUseProgram(program_simple);
        glUniformMatrix4fv(uniform_locations.simple.model, 1, GL_FALSE, glm::value_ptr(glm::identity<glm::mat4>()));

        const static GLfloat coord_lines[][3] = {
            {10.0, 0.0, 0.0},  {-10.0, 0.0, 0.0}, {10.0, 1.0, 0.0},   {-10.0, 1.0, 0.0}, {10.0, 0.0, 1.0},
            {-10.0, 0.0, 1.0}, {10.0, -1.0, 0.0}, {-10.0, -1.0, 0.0}, {10.0, 0.0, -1.0}, {-10.0, 0.0, -1.0},
            {0.0, 10.0, 0.0},  {0.0, -10.0, 0.0}, {1.0, 10.0, 0.0},   {1.0, -10.0, 0.0}, {0.0, 10.0, 1.0},
            {0.0, -10.0, 1.0}, {-1.0, 10.0, 0.0}, {-1.0, -10.0, 0.0}, {0.0, 10.0, -1.0}, {0.0, -10.0, -1.0},
            {0.0, 0.0, 10.0},  {0.0, 0.0, -10.0}, {1.0, 0.0, 10.0},   {1.0, 0.0, -10.0}, {0.0, 1.0, 10.0},
            {0.0, 1.0, -10.0}, {-1.0, 0.0, 10.0}, {-1.0, 0.0, -10.0}, {0.0, -1.0, 10.0}, {0.0, -1.0, -10.0},
        };

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, coord_lines);

        glVertexAttrib3f(2, 0.f, 0.f, 0.f);

        glDrawArrays(GL_LINES, 0, sizeof(coord_lines) / sizeof(GLfloat) / 3);

        glDisableVertexAttribArray(0);
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
        SET_SIMPLE_UNIFORM_MAT4(model);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        glVertexAttrib4fv(2, wire_color);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

        glDrawElements(GL_TRIANGLES, model.indices.size(), GL_UNSIGNED_INT, nullptr);

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
        glEnableVertexAttribArray(0);
        glDisableVertexAttribArray(2);
        glUseProgram(program_simple);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BARRIER_BIT_EXT, 0);

        auto mesh = genSolidSphere(0.05, 16, 16);

        auto draw_light = [&](int i) {
            glm::mat4 m = glm::translate(glm::identity<glm::mat4>(), glm::make_vec3(lights[i].position));
            glUniformMatrix4fv(uniform_locations.simple.model, 1, GL_FALSE, glm::value_ptr(m));
            glVertexAttrib4fv(2, lights[i].diffuse);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, mesh.vertices.data());
            glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, mesh.indices.data());
        };

        for (int i = 0; i < LIGHTS; ++i) {
            draw_light(i);
        }

        glDisableVertexAttribArray(0);
    }

    // 强调被选中的顶点
    // TODO: 在 shader 中使用 gl_PointSize 和 gl_PointCoord 绘制圆点
    void draw_selected_vertex() {
        glEnableVertexAttribArray(0);
        glDisableVertexAttribArray(2);
        glUseProgram(program_simple);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BARRIER_BIT_EXT, 0);
        glVertexAttrib3f(2, 0.0, 0.0, 0.0);

        auto mesh = genSolidSphere(0.01, 10, 10);

        glm::mat4 m = glm::translate(mat_model, glm::make_vec3(model.vertices.data() + selected_id));
        glUniformMatrix4fv(uniform_locations.simple.model, 1, GL_FALSE, glm::value_ptr(m));
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, mesh.vertices.data());
        glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, mesh.indices.data());

        glDisableVertexAttribArray(0);
    }

    // 绘制被点选的面片
    void draw_selected_face() {
        glEnableVertexAttribArray(0);
        glDisableVertexAttribArray(2);
        glUseProgram(program_simple);
        SET_SIMPLE_UNIFORM_MAT4(model);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        glVertexAttrib3f(2, 0.f, 0.f, 0.f);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

        // 使用 IBO 时，最后参数表示 IBO 中以字节为单位的偏移
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, reinterpret_cast<const void *>(selected_id * sizeof(GLuint)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDisableVertexAttribArray(0);
    }

    // 更新状态
    void update_status() {
        ImGuiIO &io = ImGui::GetIO();
        // 更新视口参数
        viewport.w  = std::min({(GLint)io.DisplaySize.x, (GLint)io.DisplaySize.y, (GLint)800});
        viewport.h  = viewport.w;
        viewport.x  = io.DisplaySize.x - viewport.w;
        viewport.y  = (io.DisplaySize.y - viewport.h) / 2;
        // 更新姿态
        lb_clicked  = false;

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
                pitch_angle = glm::mod(pitch_angle, 360.0f);
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
    }

    void set_lookat() {
        // 注意y轴朝上
        auto p = glm::radians(pitch_angle);
        glm::vec3 eye;
        eye.x = view_distance * sqrt(0.5f) * sin(p);
        eye.y = view_distance * cos(p);
        eye.z = view_distance * sqrt(0.5f) * sin(p);

        glm::vec3 up = glm::cross(eye, {1.0f, 0.0f, -1.0f});

        mat_view = glm::lookAt(eye, glm::vec3(0.0f, 0.0f, 0.0f), up);
        glMultMatrixf(glm::value_ptr(mat_view));
    }

    // 执行选取
    // TODO: 使用软件实现
    void do_select() {
        ImGuiIO &io = ImGui::GetIO();
        glUseProgram(0);
        glSelectBuffer(SELECT_BUF_SIZE, select_buffer);
        glRenderMode(GL_SELECT);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();

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
        GLint hits = glRenderMode(GL_RENDER);
        printf("hits: %d\n", hits);
        if (hits >= 1) {
            GLuint minz = select_buffer[1];
            selected_id = select_buffer[3];
            size_t i    = 0;
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
                        minz        = select_buffer[i + 1];
                        selected_id = select_buffer[i + 3];
                    }
                    i += 3 + select_buffer[i];
                }
            }
            pick_sucess = true;
            printf("selected id: %d\n", selected_id);
        }
    }

    // UI 设计代码
    void design_gui() {
        ImGuiIO &io = ImGui::GetIO();
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
                ImGui::ColorEdit4("ambient", material.ambient);
                ImGui::ColorEdit4("diffuse", material.diffuse);
                ImGui::ColorEdit4("specular", material.specular);
                ImGui::SliderFloat("shininess", &material.shininess, 0, 128);
                ImGui::Separator();
                ImGui::Text("built-in materials");
                // Manually wrapping
                ImGuiStyle &style               = ImGui::GetStyle();
                constexpr size_t material_count = sizeof(materials) / sizeof(Material);
                static std::vector<float> button_widths(material_count);
                float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
                for (size_t i = 0; i < material_count; ++i) {
                    const auto &m = materials[i];
                    if (ImGui::Button(m.name)) {
                        material = m;
                    }
                    button_widths[i]     = ImGui::GetItemRectSize().x;
                    float last_button_x2 = ImGui::GetItemRectMax().x;
                    // Expected position if next button was on same line
                    if (i + 1 < material_count) {
                        float next_button_x2 = last_button_x2 + style.ItemSpacing.x + button_widths[i + 1];
                        if (next_button_x2 < window_visible_x2)
                            ImGui::SameLine();
                    }
                }
                ImGui::EndTabItem();
            }
            for (int i = 0; i < LIGHTS; ++i) {
                char light_tabname[10];
                sprintf(light_tabname, "Light%d", i);
                if (ImGui::BeginTabItem(light_tabname)) {
                    ImGui::ColorEdit4("ambient", lights[i].ambient);
                    ImGui::ColorEdit4("diffuse", lights[i].diffuse);
                    ImGui::ColorEdit4("specular", lights[i].specular);
                    ImGui::Text("position:");
                    ImGui::SliderFloat("x", &lights[i].position[0], -5.0f, 5.0f);
                    ImGui::SliderFloat("y", &lights[i].position[1], -5.0f, 5.0f);
                    ImGui::SliderFloat("z", &lights[i].position[2], -5.0f, 5.0f);
                    ImGui::EndTabItem();
                }
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

        // 显示拾取结果数据的弹窗
        // popup 窗口触发后状态由 IMGUI 自动管理
        // 所以只需要触发一次
        if (pick_sucess) {
            ImGui::OpenPopup("#select popup");
        }
        ImVec2 popup_pos(lb_press_pos.x + 10, lb_press_pos.y - 10);
        ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Always, ImVec2(0.0, 1.0));
        // select_dispaly 与 popup 窗口状态保持一致
        select_dispaly = ImGui::BeginPopup("#select popup");
        if (select_dispaly) {
            if (select_mode == SELECT_VERTEX) {
                ImGui::Text("vertex %d", selected_id / 3);
                ImGui::Text("(%f, %f, %f)", model.vertices[selected_id], model.vertices[selected_id + 1],
                            model.vertices[selected_id + 2]);
            }
            if (select_mode == SELECT_FACE) {
                auto v1 = model.indices[selected_id];
                auto v2 = model.indices[selected_id + 1];
                auto v3 = model.indices[selected_id + 2];
                ImGui::Text("triangle %d", selected_id / 3);
                ImGui::Text("v1: (%f, %f, %f)", model.vertices[v1], model.vertices[v1 + 1], model.vertices[v1 + 2]);
                ImGui::Text("v2: (%f, %f, %f)", model.vertices[v2], model.vertices[v2 + 1], model.vertices[v2 + 2]);
                ImGui::Text("v3: (%f, %f, %f)", model.vertices[v3], model.vertices[v3 + 1], model.vertices[v3 + 2]);
            }
            ImGui::EndPopup();
        }
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
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        // 更新状态
        update_status();

        // 设置视口
        // 在 rentia 这样的屏幕上需要如此适配，从逻辑像素得到实际像素
        {
            float x = viewport.x * io.DisplayFramebufferScale.x;
            float y = viewport.y * io.DisplayFramebufferScale.y;
            float w = viewport.w * io.DisplayFramebufferScale.x;
            float h = viewport.h * io.DisplayFramebufferScale.y;
            glViewport(x, y, w, h);
        }

        // 设置模型姿态
        set_model_transform();

        // 拾取模式
        pick_sucess = false;
        if (lb_clicked && select_mode != SELECT_NONE) {
            do_select();
        }

        // Start the Dear ImGui frame
        ImGui::NewFrame();

        design_gui();

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

        mat_proj = glm::perspective(glm::radians(fovy), 1.0f, 0.1f, 1000.0f);
        glLoadMatrixf(glm::value_ptr(mat_proj));

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        set_lookat();

        // 光源及材质设置
        set_phong_uniform();

        // 简单着色器参数设置
        set_simple_uniform();

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
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
}

void draw_model_select_vertex() {
    model_transform();

    glInitNames();
    glPushName(-1);
    auto vd = model.vertices.data();
    for (size_t v = 0; v < model.vertices.size(); v += 3) {
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
    auto vd = model.vertices.data();
    auto fd = model.indices.data();
    for (size_t f = 0; f < model.indices.size(); f += 3) {
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
