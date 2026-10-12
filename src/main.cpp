// CS 6983 Module 2 Week 3
// Sample OpenGL Program

#include "gl_loader.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct App {
    // screen size
    int width = 1920;
    int height = 1080;
    // initial camera position
    float yaw = 0.7f;
    float pitch = 0.4f;
    float distance = 6.0f;
    // dragging state
    bool dragging = false;
    // last mouse position
    double lastX = 0.0;
    double lastY = 0.0;
};

static App gApp;

static GLuint gProgram = 0;
static GLint gMvpLoc = -1;
static GLint gColorLoc = -1;
static GLuint gCubeVao = 0;
static GLuint gCubeVertexCount = 0;
static GLuint gGridVao = 0;
static GLuint gGridVertexCount = 0;

// colors
const glm::vec3 LIGHT_GRAY(0.28f, 0.31f, 0.34f); // Gray
const glm::vec3 DARK_GRAY(0.11f, 0.12f, 0.14f); // Dark Gray
const glm::vec3 YELLOW(0.83f, 0.64f, 0.36f); // Yellow
const glm::vec3 BLUE(0.32f, 0.58f, 0.78f); // Blue
const glm::vec3 RED(0.80f, 0.36f, 0.36f); // Red

// ###########################################################################################
// Shaders
// ###########################################################################################

// helper function to read the text from a file
// Thanks Cursor AI
static std::string read_text(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Could not open " + path);
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

// Thanks Cursor AI
static GLuint compile_shader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* text = source.c_str();
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << log << "\n";
        throw std::runtime_error("Shader compile failed");
    }
    return shader;
}

// Thanks Cursor AI
static GLuint link_program(const std::string& vertPath, const std::string& fragPath) {
    GLuint vert = compile_shader(GL_VERTEX_SHADER, read_text(vertPath));
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, read_text(fragPath));
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << log << "\n";
        throw std::runtime_error("Program link failed");
    }
    return program;
}

// ###########################################################################################
// Meshes
// ###########################################################################################


static GLuint upload_positions(const std::vector<float>& positions) {
    GLuint vao = 0; // vertex array object: how to interpret the vertex data
    GLuint vbo = 0; // vertex buffer object: where to store the vertex data
    // generate the vertex array and buffer
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    // bind the vertex array and buffer
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // upload the data to the buffer
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(positions.size() * sizeof(float)), positions.data(), GL_STATIC_DRAW);
    // set the vertex attribute pointer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    // enable the vertex attribute array
    glEnableVertexAttribArray(0);
    // return the vertex array
    return vao;
}

// Thanks Cursor AI
static void add_quad(std::vector<float>& dst,
                      const glm::vec3& a,
                      const glm::vec3& b,
                      const glm::vec3& c,
                      const glm::vec3& d) {
    // define the corners of the quad
    const glm::vec3 corners[6] = {a, b, c, a, c, d};
    // add the corners to the vector
    for (const glm::vec3& p : corners) {
        dst.push_back(p.x);
        dst.push_back(p.y); 
        dst.push_back(p.z); 
    }
}

// Thanks Cursor AI
static GLuint make_cube() {
    // define the vertices of the cube
    const glm::vec3 p[8] = {
        {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f},
        {0.5f, 0.5f, -0.5f},   {-0.5f, 0.5f, -0.5f},
        {-0.5f, -0.5f, 0.5f},  {0.5f, -0.5f, 0.5f},
        {0.5f, 0.5f, 0.5f},    {-0.5f, 0.5f, 0.5f},
    };
    // create a vector to store the vertices
    std::vector<float> positions;
    // add the faces of the cube
    add_quad(positions, p[0], p[1], p[2], p[3]);
    add_quad(positions, p[5], p[4], p[7], p[6]);
    add_quad(positions, p[4], p[0], p[3], p[7]);
    add_quad(positions, p[1], p[5], p[6], p[2]);
    add_quad(positions, p[3], p[2], p[6], p[7]);
    add_quad(positions, p[4], p[5], p[1], p[0]);
    // set the number of vertices
    gCubeVertexCount = static_cast<GLuint>(positions.size() / 3);
    return upload_positions(positions);
}

// Thanks Cursor AI
static GLuint make_grid() {
    // create a vector to store the vertices
    std::vector<float> positions;
    // create the vertices of the grid
    for (int i = -4; i <= 4; ++i) {
        const float x = static_cast<float>(i);
        positions.insert(positions.end(), {x, 0.0f, -4.0f, x, 0.0f, 4.0f});
        positions.insert(positions.end(), {-4.0f, 0.0f, x, 4.0f, 0.0f, x});
    }
    // set the number of vertices
    gGridVertexCount = static_cast<GLuint>(positions.size() / 3);
    return upload_positions(positions);
}

static void draw_mesh(GLuint vao, GLuint count, GLenum mode, const glm::mat4& mvp, const glm::vec3& color) {
    // set the Model-View-Projection matrix
    // glUniformMatrix4fv(location, count, transpose, value)
    glUniformMatrix4fv(gMvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    // pass color to the fragment shader
    glUniform3f(gColorLoc, color.r, color.g, color.b);
    // bind the vertex array
    glBindVertexArray(vao);
    // draw the vertices
    glDrawArrays(mode, 0, static_cast<GLint>(count));
}

// ###########################################################################################
// Transform
// ###########################################################################################

// Unity Transform: translate, then rotate, then scale. Scale applies first
// to the vertex because the vector sits on the right: M * p = T * R * S * p.
static glm::mat4 model_matrix(const glm::vec3& position, float radians, const glm::vec3& scale) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, radians, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, scale);
    return model;
}

// ###########################################################################################
// Camera
// ###########################################################################################

// Unity Camera position and orientation
static glm::mat4 view_matrix() {
    const float cosPitch = std::cos(gApp.pitch);
    // eye position is calculated based on the pitch and yaw angles
    const glm::vec3 eye(
        gApp.distance * cosPitch * std::sin(gApp.yaw), // x coordinate
        gApp.distance * std::sin(gApp.pitch), // y coordinate
        gApp.distance * cosPitch * std::cos(gApp.yaw) // z coordinate
    );
    // return the view matrix
    // lookAt(camera position, target position, up vector)
    return glm::lookAt(eye, glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

// Unity Camera field of view, aspect, near plane, far plane
static glm::mat4 projection_matrix() {
    // calculate the aspect ratio
    const float aspect = gApp.height > 0
        ? static_cast<float>(gApp.width) / static_cast<float>(gApp.height)
        : 1.0f;

    // 70 degrees is the camera field of view (FOV)
    float fov = 40.0f;      
    // set clipping planes
    // near plane must stay above 0. camera-space z = 0 cannot be projected
    float near = 0.1f;
    float far = 15.0f;
    return glm::perspective(glm::radians(fov), aspect, near, far);
}

// ###########################################################################################
// Scene
// ###########################################################################################

static void draw_scene(float time) {
    // calculate the view-projection matrix
    const glm::mat4 vp = projection_matrix() * view_matrix();

    // draw the grid using lines
    draw_mesh(gGridVao, gGridVertexCount, GL_LINES, vp, LIGHT_GRAY);

    // draw the near cube first using triangles
    const glm::mat4 nearModel = model_matrix(glm::vec3(0.0f, 0.5f, 0.0f), time * 0.6f, glm::vec3(1.0f));
    draw_mesh(gCubeVao, gCubeVertexCount, GL_TRIANGLES, vp * nearModel, YELLOW);

    // draw the far cube second
    // the depth test keeps the nearer fragment, so the yellow cube still wins where the two overlap
    const glm::mat4 farModel = model_matrix(glm::vec3(-2.5f, 0.25f, -1.7f), 0.0f, glm::vec3(0.5f));
    draw_mesh(gCubeVao, gCubeVertexCount, GL_TRIANGLES, vp * farModel, BLUE);
}

// ###########################################################################################
// Callbacks functions
// ###########################################################################################

static void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    gApp.width = width;
    gApp.height = height;
}

// ###########################################################################################
// Input Callbacks
// ###########################################################################################

// Thanks Cursor AI
static void key_callback(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) {
        return;
    }
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else if (key == GLFW_KEY_LEFT) {
        gApp.yaw -= 0.08f;
    } else if (key == GLFW_KEY_RIGHT) {
        gApp.yaw += 0.08f;
    } else if (key == GLFW_KEY_UP) {
        gApp.pitch += 0.06f;
    } else if (key == GLFW_KEY_DOWN) {
        gApp.pitch -= 0.06f;
    } else if (key == GLFW_KEY_MINUS) {
        gApp.distance += 0.3f;
    } else if (key == GLFW_KEY_EQUAL) {
        gApp.distance -= 0.3f;
    }
}

// Thanks Cursor AI
static void mouse_button_callback(GLFWwindow*, int button, int action, int) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        gApp.dragging = action == GLFW_PRESS;
    }
}

// Thanks Cursor AI
static void cursor_callback(GLFWwindow*, double x, double y) {
    if (gApp.dragging) {
        gApp.yaw += static_cast<float>(x - gApp.lastX) * 0.008f;
        gApp.pitch += static_cast<float>(y - gApp.lastY) * 0.008f;
    }
    gApp.lastX = x;
    gApp.lastY = y;
}

// Thanks Cursor AI
static void scroll_callback(GLFWwindow*, double, double yOffset) {
    gApp.distance -= static_cast<float>(yOffset) * 0.4f;
}

// ###########################################################################################
// Camera Clamping
// ###########################################################################################

// Thanks Cursor AI
static void clamp_camera() {
    if (gApp.pitch > 1.15f) gApp.pitch = 1.15f;
    if (gApp.pitch < -1.15f) gApp.pitch = -1.15f;
    if (gApp.distance < 2.0f) gApp.distance = 2.0f;
    if (gApp.distance > 18.0f) gApp.distance = 18.0f;
}

// ###########################################################################################
// Main
// ###########################################################################################

int main() {
    glfwSetErrorCallback([](int code, const char* description) {
        std::cerr << "GLFW " << code << ": " << description << "\n";
    });
    if (!glfwInit()) {
        return 1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    // create the window
    GLFWwindow* window = glfwCreateWindow(gApp.width, gApp.height, "CS 6983 OpenGL", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    // make the window the current context so we can draw to it
    glfwMakeContextCurrent(window);
    // wait for the display to refresh before swapping the buffers
    glfwSwapInterval(1);
    // set the callback functions for the window
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwGetFramebufferSize(window, &gApp.width, &gApp.height);

    // run the driver 
    if (!load_gl()) {
        glfwTerminate();
        return 1;
    }

    // link the shaders
    try {
        gProgram = link_program(std::string(SHADER_DIR) + "/color_vert.glsl",
                                std::string(SHADER_DIR) + "/color_frag.glsl");
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\n";
        glfwTerminate();
        return 1;
    }
    // get the location of the uniform variables
    gMvpLoc = glGetUniformLocation(gProgram, "uMVP");
    gColorLoc = glGetUniformLocation(gProgram, "uColor");

    // make the meshes
    gCubeVao = make_cube();
    gGridVao = make_grid();
    // use the program
    glUseProgram(gProgram);
    // enable the depth test
    glEnable(GL_DEPTH_TEST);
    // set the background color
    glClearColor(DARK_GRAY.r, DARK_GRAY.g, DARK_GRAY.b, 1.0f);
    //glClearColor(RED.r, RED.g, RED.b, 1.0f);

    std::cout << "drag or arrows to orbit, wheel or -/= to zoom, Esc quits\n";
    // main loop
    while (!glfwWindowShouldClose(window)) {
        // poll for events (inputs)
        glfwPollEvents();
        // get the time
        const float time = static_cast<float>(glfwGetTime());
        // clamp the camera to prevent it from going too far
        clamp_camera();
        
        // set the viewport and clear the color and depth buffers to prepare for the next frame
        glViewport(0, 0, gApp.width, gApp.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw the scene 
        draw_scene(time);
        // swap the buffers to display the new frame
        glfwSwapBuffers(window);
    }

    // terminate the window
    glfwTerminate();
    return 0;
}
