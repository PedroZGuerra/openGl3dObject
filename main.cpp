#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <functional>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const unsigned int WINDOW_WIDTH = 800;
const unsigned int WINDOW_HEIGHT = 600;
const char* WINDOW_TITLE = "Modelo Earth";

// Configurações iniciais da câmera
glm::vec3 cameraPos(0.0f, 0.0f, 50.0f);
glm::vec3 cameraFront(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
float yaw = -90.0f, pitch = 0.0f, fov = 45.0f;
float lastX = 800.0f / 2.0f, lastY = 600.0f / 2.0f;
bool firstMouse = true;

// Configurações de luz
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
float lightIntensity = 1.0f;

// Estrutura de vértices
struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

// Callback para redimensionar a janela
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Processamento de entrada do teclado
void processInput(GLFWwindow* window) {
    const float cameraSpeed = 0.4f;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    // Ajustar intensidade da luz
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        lightIntensity = glm::min(lightIntensity + 0.1f, 2.0f);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        lightIntensity = glm::max(lightIntensity - 0.1f, 0.0f);
}

// Callback para movimento do mouse
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    const float sensitivity = 0.1f;
    yaw += xOffset * sensitivity;
    pitch += yOffset * sensitivity;

    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

// Callback para rolagem do mouse
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    fov = glm::clamp(fov - (float)yoffset, 1.0f, 45.0f);
}

// Função para carregar um modelo com Assimp
void loadModel(const std::string& path, std::vector<Vertex>& vertices, std::vector<GLuint>& indices) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }

    std::function<void(aiNode*, const aiScene*)> processNode;
    processNode = [&](aiNode* node, const aiScene* scene) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

            // Processar vértices
            for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
                Vertex vertex;
                vertex.Position = { mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z };
                vertex.Normal = mesh->HasNormals() ?
                    glm::vec3(mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z)
                    : glm::vec3(0.0f, 0.0f, 0.0f);

                if (mesh->mTextureCoords[0]) {
                    vertex.TexCoords = {
                        mesh->mTextureCoords[0][j].x,
                        mesh->mTextureCoords[0][j].y
                    };
                }
                else {
                    vertex.TexCoords = glm::vec2(0.0f);
                }
                vertices.push_back(vertex);
            }

            // Processar índices
            for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
                const aiFace& face = mesh->mFaces[j];
                indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
            }
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++)
            processNode(node->mChildren[i], scene);
        };

    processNode(scene->mRootNode, scene);
}

// Função para carregar e compilar shaders
GLuint createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath) {
    auto readFile = [](const std::string& path) -> std::string {
        std::ifstream file(path);
        if (!file.is_open())
            throw std::runtime_error("Failed to open file: " + path);
        return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
        };

    auto compileShader = [](GLenum type, const std::string& source) -> GLuint {
        GLuint shader = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            throw std::runtime_error("Shader compilation error: " + std::string(infoLog));
        }
        return shader;
        };

    std::string vertexCode = readFile(vertexPath);
    std::string fragmentCode = readFile(fragmentPath);

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexCode);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentCode);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
        throw std::runtime_error("Shader linking error: " + std::string(infoLog));
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// Função para inicializar GLFW e criar a janela
GLFWwindow* initializeGLFW() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Necessário para MacOS

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);
    return window;
}

// Função para inicializar GLEW
bool initializeGLEW() {
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();

    // Ignorar erro 1280 gerado pelo GLEW
    glGetError();
    if (err != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    return true;
}

// Função para configurar os buffers do modelo
void setupModelBuffers(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices, GLuint& VAO, GLuint& VBO, GLuint& EBO) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

// Função para verificar índices do modelo
bool validateModelIndices(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices) {
    for (size_t i = 0; i < indices.size(); ++i) {
        if (indices[i] >= vertices.size()) {
            std::cerr << "ERROR: Index out of bounds at position " << i << ": " << indices[i] << std::endl;
            return false;
        }
    }
    return true;
}

// Função para carregar a textura
GLuint loadTexture(const std::string& texturePath) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cerr << "Failed to load texture: " << texturePath << std::endl;
    }

    stbi_image_free(data);
    return texture;
}

int main() {
    // Inicializar GLFW e criar a janela
    GLFWwindow* window = initializeGLFW();
    if (!window) return -1;

    // Configurar callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Inicializar GLEW
    if (!initializeGLEW()) {
        glfwTerminate();
        return -1;
    }

    // Carregar textura
    GLuint texture = loadTexture("C:\\faculdade\\compgraf\\trab2011\\CG_ProjTerreno\\Earth_diff.jpg");

    // Criar o programa de shader
    GLuint shaderProgram = createShaderProgram(
        "C:\\faculdade\\compgraf\\trab2011\\CG_ProjTerreno\\vertex_shader.vert",
        "C:\\faculdade\\compgraf\\trab2011\\CG_ProjTerreno\\fragment_shader.frag");

    // Localizações de uniformes
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    GLint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
    GLint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
    GLint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    GLint texture1Loc = glGetUniformLocation(shaderProgram, "texture1");

    // Carregar modelo
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    loadModel("C:\\faculdade\\compgraf\\trab2011\\CG_ProjTerreno\\13902_Earth_v1_l3.obj", vertices, indices);

    if (vertices.empty() || indices.empty()) {
        std::cerr << "ERROR: Model data is empty. Exiting." << std::endl;
        glfwTerminate();
        return -1;
    }

    if (!validateModelIndices(vertices, indices)) {
        glfwTerminate();
        return -1;
    }

    // Configurar buffers
    GLuint VAO, VBO, EBO;
    setupModelBuffers(vertices, indices, VAO, VBO, EBO);

    // Loop de renderização
    glm::vec3 lightPos(10.0f, 10.0f, 10.0f);
    glm::vec3 objectColor(1.0f, 1.0f, 1.0f);

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);

        // Matrizes de transformação
        glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f));
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 1000.0f);

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Atualizar luz e textura
        glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));
        glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor * lightIntensity));
        glUniform3fv(objectColorLoc, 1, glm::value_ptr(objectColor));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(texture1Loc, 0);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Liberar recursos
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &texture);
    glfwTerminate();
    return 0;
}