#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// --- Configuration Constants ---
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const char* APP_TITLE = "Coal Summer Octopus";

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define RADIANS(deg) ((deg) * (float)M_PI / 180.0f)

// --- Global Camera Control Variables ---
static double mouseX = 0.0, mouseY = 0.0;
static double lastMouseX = 0.0, lastMouseY = 0.0;
static int isLeftMousePressed = 0;

static float cameraDistance = 3.0f;      // Zoom distance
static float cameraYaw      = 0.0f;      // Left-right rotation (radians)
static float cameraPitch    = 0.5f;      // Up-down rotation (radians)

static const float MIN_DISTANCE = 1.5f;
static const float MAX_DISTANCE = 10.0f;
static const float ZOOM_SENSITIVITY = 0.5f;
static const float ROTATE_SENSITIVITY = 0.005f;

// --- Math Implementation (GLM replacement) ---

typedef struct { float m[4][4]; } mat4;
typedef struct { float m[3][3]; } mat3;
typedef struct { float x, y, z; } vec3;

// Create identity matrix
static inline mat4 mat4_identity() {
    mat4 res = {0};
    for(int i=0; i<4; i++) res.m[i][i] = 1.0f;
    return res;
}

// Matrix multiplication (A * B)
static inline mat4 mat4_mul(mat4 a, mat4 b) {
    mat4 res = {0};
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) sum += a.m[r][k] * b.m[k][c];
            res.m[r][c] = sum;
        }
    }
    return res;
}

// Perspective
static inline mat4 mat4_perspective(float fovy, float aspect, float near, float far) {
    mat4 res = {0};
    float tanHalfFovy = tanf(fovy / 2.0f);
    
    res.m[0][0] = 1.0f / (aspect * tanHalfFovy);
    res.m[1][1] = 1.0f / (tanHalfFovy);
    res.m[2][2] = -(far + near) / (far - near);
    res.m[2][3] = -(2.0f * far * near) / (far - near);
    res.m[3][2] = -1.0f; 
    res.m[3][3] = 0.0f;
    return res;
}

// Rotation Y
static inline mat4 mat4_rotate_y(mat4 m, float angle) {
    mat4 rot = mat4_identity();
    float c = cosf(angle);
    float s = sinf(angle);
    rot.m[0][0] = c;
    rot.m[0][2] = s;
    rot.m[2][0] = -s;
    rot.m[2][2] = c;
    return mat4_mul(m, rot); 
}

// Rotation X
static inline mat4 mat4_rotate_x(mat4 m, float angle) {
    mat4 rot = mat4_identity();
    float c = cosf(angle);
    float s = sinf(angle);
    rot.m[1][1] = c;
    rot.m[1][2] = -s;
    rot.m[2][1] = s;
    rot.m[2][2] = c;
    return mat4_mul(m, rot);
}

// --- Vector Math for Camera ---

static inline vec3 vec3_norm(vec3 v) {
    float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if(len < 1e-6f) return v;
    vec3 res = { v.x / len, v.y / len, v.z / len };
    return res;
}

static inline vec3 vec3_cross(vec3 a, vec3 b) {
    vec3 res;
    res.x = a.y * b.z - a.z * b.y;
    res.y = a.z * b.x - a.x * b.z;
    res.z = a.x * b.y - a.y * b.x;
    return res;
}

static inline vec3 vec3_sub(vec3 a, vec3 b) {
    return (vec3){ a.x - b.x, a.y - b.y, a.z - b.z };
}

static inline float vec3_dot(vec3 a, vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Extract mat3 from mat4
static inline mat3 mat4_to_mat3(mat4 m) {
    mat3 res;
    for(int i=0; i<3; i++)
        for(int j=0; j<3; j++)
            res.m[i][j] = m.m[i][j];
    return res;
}

static inline mat3 mat3_transpose(mat3 m) {
    mat3 res;
    for(int i=0; i<3; i++)
        for(int j=0; j<3; j++)
            res.m[i][j] = m.m[j][i];
    return res;
}

static inline mat3 mat3_inverse(mat3 m) {
    float det = m.m[0][0] * (m.m[1][1] * m.m[2][2] - m.m[2][1] * m.m[1][2]) -
                m.m[0][1] * (m.m[1][0] * m.m[2][2] - m.m[2][0] * m.m[1][2]) +
                m.m[0][2] * (m.m[1][0] * m.m[2][1] - m.m[1][1] * m.m[2][0]);

    if (fabsf(det) < 1e-6f) return mat4_to_mat3(mat4_identity());

    float invDet = 1.0f / det;
    mat3 res;
    res.m[0][0] =  (m.m[1][1] * m.m[2][2] - m.m[2][1] * m.m[1][2]) * invDet;
    res.m[0][1] = -(m.m[0][1] * m.m[2][2] - m.m[2][1] * m.m[0][2]) * invDet;
    res.m[0][2] =  (m.m[0][1] * m.m[2][1] - m.m[1][1] * m.m[0][2]) * invDet;
    res.m[1][0] = -(m.m[1][0] * m.m[2][2] - m.m[2][0] * m.m[1][2]) * invDet;
    res.m[1][1] =  (m.m[0][0] * m.m[2][2] - m.m[2][0] * m.m[0][2]) * invDet;
    res.m[1][2] = -(m.m[0][0] * m.m[2][1] - m.m[2][0] * m.m[0][1]) * invDet;
    res.m[2][0] =  (m.m[1][0] * m.m[2][1] - m.m[2][0] * m.m[1][1]) * invDet;
    res.m[2][1] = -(m.m[0][0] * m.m[2][1] - m.m[2][0] * m.m[0][1]) * invDet;
    res.m[2][2] =  (m.m[0][0] * m.m[1][1] - m.m[1][0] * m.m[0][1]) * invDet;
    return res;
}

// LookAt Function (Correct Implementation)
static inline mat4 mat4_lookat(vec3 eye, vec3 center, vec3 up) {
    vec3 f = vec3_norm(vec3_sub(center, eye)); // Forward (Eye -> Center)
    vec3 s = vec3_norm(vec3_cross(f, up));     // Right
    vec3 u = vec3_cross(s, f);                 // Up (Recalculated)

    mat4 res = mat4_identity();
    // Rotation part
    res.m[0][0] = s.x;  res.m[0][1] = s.y;  res.m[0][2] = s.z;
    res.m[1][0] = u.x;  res.m[1][1] = u.y;  res.m[1][2] = u.z;
    res.m[2][0] =-f.x;  res.m[2][1] =-f.y;  res.m[2][2] =-f.z; // OpenGL looks down -Z

    // Translation part (Dot products)
    res.m[0][3] = -vec3_dot(s, eye);
    res.m[1][3] = -vec3_dot(u, eye);
    res.m[2][3] =  vec3_dot(f, eye); 
    
    return res;
}

// --- Callbacks (implementation from colleague) ---

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (isLeftMousePressed) {
        double deltaX = xpos - lastMouseX;
        double deltaY = ypos - lastMouseY;

        cameraYaw   -= (float)(deltaX * ROTATE_SENSITIVITY);
        
        // Inverted Y control: now += instead of -=
        // When dragging up (deltaY is negative), Pitch decreases -> camera goes down
        cameraPitch += (float)(deltaY * ROTATE_SENSITIVITY);

        // Pitch limit (to prevent camera flipping)
        const float PITCH_LIMIT = RADIANS(89.0f);
        if (cameraPitch > PITCH_LIMIT)  cameraPitch = PITCH_LIMIT;
        if (cameraPitch < -PITCH_LIMIT) cameraPitch = -PITCH_LIMIT;
    }
    lastMouseX = xpos;
    lastMouseY = ypos;
    // Update position for the next frame even if button is not pressed,
    // to avoid a jump when clicking
    if (!isLeftMousePressed) {
        mouseX = xpos; 
        mouseY = ypos;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            isLeftMousePressed = 1;
            // Read position immediately upon press
            glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
        } else if (action == GLFW_RELEASE) {
            isLeftMousePressed = 0;
        }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    cameraDistance -= (float)yoffset * ZOOM_SENSITIVITY;
    if (cameraDistance < MIN_DISTANCE) cameraDistance = MIN_DISTANCE;
    if (cameraDistance > MAX_DISTANCE) cameraDistance = MAX_DISTANCE;
}

// --- Shaders (unchanged) ---

const char* const vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aNormal;\n"
    "\n"
    "out vec3 FragPos;\n"
    "out vec3 Normal;\n"
    "\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "uniform mat3 normalMatrix;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    FragPos = vec3(model * vec4(aPos, 1.0));\n"
    "    Normal = normalize(normalMatrix * aNormal);\n"
    "    gl_Position = projection * view * vec4(FragPos, 1.0);\n"
    "}\0";

const char* const fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "\n"
    "in vec3 Normal;\n"
    "in vec3 FragPos;\n"
    "\n"
    "uniform vec3 lightPos;\n"
    "uniform vec3 viewPos;\n"
    "uniform vec3 lightColor;\n"
    "uniform vec3 objectColor;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    float ambientStrength = 0.1;\n"
    "    float specularStrength = 0.5;\n"
    "    float gamma = 2.2;\n"
    "\n"
    "    vec3 ambient = ambientStrength * lightColor;\n"
    "  \n"
    "    vec3 norm = normalize(Normal);\n"
    "    vec3 lightDir = normalize(lightPos - FragPos);\n"
    "    float diff = max(dot(norm, lightDir), 0.0);\n"
    "    vec3 diffuse = diff * lightColor;\n"
    "    \n"
    "    vec3 viewDir = normalize(viewPos - FragPos);\n"
    "    vec3 reflectDir = reflect(-lightDir, norm);\n"
    "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n"
    "    vec3 specular = specularStrength * spec * lightColor;\n"
    "        \n"
    "    vec3 result = (ambient + diffuse + specular) * objectColor;\n"
    "    result = pow(result, vec3(1.0 / gamma));\n"
    "    FragColor = vec4(result, 1.0);\n"
    "}\n\0";

typedef struct {
    GLuint ID;
    GLint modelLoc;
    GLint viewLoc;
    GLint projLoc;
    GLint normalMatrixLoc;
    GLint lightPosLoc;
    GLint viewPosLoc;
    GLint lightColorLoc;
    GLint objectColorLoc;
} ShaderProgram;

void checkCompileErrors(unsigned int shader, const char* type) {
    int success;
    char infoLog[1024];
    if (strcmp(type, "PROGRAM") != 0) {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            printf("ERROR::SHADER_COMPILATION_ERROR of type: %s\n%s\n", type, infoLog);
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            printf("ERROR::PROGRAM_LINKING_ERROR of type: %s\n%s\n", type, infoLog);
        }
    }
}

void shader_init(ShaderProgram* self, const char* vSource, const char* fSource) {
    unsigned int vertex, fragment;
    
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vSource, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fSource, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    self->ID = glCreateProgram();
    glAttachShader(self->ID, vertex);
    glAttachShader(self->ID, fragment);
    glLinkProgram(self->ID);
    checkCompileErrors(self->ID, "PROGRAM");

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    glUseProgram(self->ID);
    self->modelLoc = glGetUniformLocation(self->ID, "model");
    self->viewLoc = glGetUniformLocation(self->ID, "view");
    self->projLoc = glGetUniformLocation(self->ID, "projection");
    self->normalMatrixLoc = glGetUniformLocation(self->ID, "normalMatrix");
    self->lightPosLoc = glGetUniformLocation(self->ID, "lightPos");
    self->viewPosLoc = glGetUniformLocation(self->ID, "viewPos");
    self->lightColorLoc = glGetUniformLocation(self->ID, "lightColor");
    self->objectColorLoc = glGetUniformLocation(self->ID, "objectColor");
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// --- Main ---
int main() {
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); 

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, APP_TITLE, NULL, NULL);
    if (!window) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Register new callbacks
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    ShaderProgram shader;
    shader_init(&shader, vertexShaderSource, fragmentShaderSource);

    // Pyramid data
    float vertices[] = {
        // Base
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,
        // Front
         0.0f,  0.5f,  0.0f,  0.0f, 0.4472f,  0.8944f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.4472f,  0.8944f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.4472f,  0.8944f,
        // Right
         0.0f,  0.5f,  0.0f,  0.8944f, 0.4472f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.8944f, 0.4472f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.8944f, 0.4472f,  0.0f,
        // Back
         0.0f,  0.5f,  0.0f,  0.0f, 0.4472f, -0.8944f,
         0.5f, -0.5f, -0.5f,  0.0f, 0.4472f, -0.8944f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.4472f, -0.8944f,
        // Left
         0.0f,  0.5f,  0.0f, -0.8944f, 0.4472f,  0.0f,
        -0.5f, -0.5f, -0.5f, -0.8944f, 0.4472f,  0.0f,
        -0.5f, -0.5f,  0.5f, -0.8944f, 0.4472f,  0.0f
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0);

    vec3 lightPos = {1.2f, 1.0f, 2.0f};

    // --- Pyramid Rotation Variables ---
    float currentRotation = 0.0f;
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        // Calculate deltaTime
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, 1);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader.ID);
        
        // Update pyramid rotation (0.5 radians per second)
        currentRotation += 0.5f * deltaTime;

        glUniform3f(shader.objectColorLoc, 1.0f, 0.5f, 0.31f);
        glUniform3f(shader.lightColorLoc, 1.0f, 1.0f, 1.0f);
        glUniform3f(shader.lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
        
        // --- View Matrix Calculation (Orbit Camera) ---
        float camX = cameraDistance * cosf(cameraPitch) * sinf(cameraYaw);
        float camY = cameraDistance * sinf(cameraPitch);
        float camZ = cameraDistance * cosf(cameraPitch) * cosf(cameraYaw);
        
        vec3 cameraPos = { camX, camY, camZ };
        vec3 cameraTarget = { 0.0f, 0.0f, 0.0f };
        vec3 cameraUp = { 0.0f, 1.0f, 0.0f };

        mat4 view = mat4_lookat(cameraPos, cameraTarget, cameraUp);
        
        glUniformMatrix4fv(shader.viewLoc, 1, GL_TRUE, &view.m[0][0]);
        glUniform3f(shader.viewPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);

        // --- Projection ---
        mat4 projection = mat4_perspective(RADIANS(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(shader.projLoc, 1, GL_TRUE, &projection.m[0][0]);

        // --- Model (Rotating + Tilt) ---
        mat4 model = mat4_identity();
        
        // First rotate around Y (auto-rotation)
        model = mat4_rotate_y(model, currentRotation);
        // Then tilt along X
        model = mat4_rotate_x(model, RADIANS(20.0f));
        
        glUniformMatrix4fv(shader.modelLoc, 1, GL_TRUE, &model.m[0][0]);

        // Normal Matrix
        mat3 normalMatrix = mat4_to_mat3(model);
        normalMatrix = mat3_inverse(normalMatrix);
        normalMatrix = mat3_transpose(normalMatrix);
        
        glUniformMatrix3fv(shader.normalMatrixLoc, 1, GL_TRUE, &normalMatrix.m[0][0]);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 18);
        glBindVertexArray(0);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shader.ID);

    glfwTerminate();
    return 0;
}