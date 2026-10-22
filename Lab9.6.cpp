
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader_m.h"

#include <iostream>
#include <cmath>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

glm::vec3 cameraPos(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);

bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 400.0f;
float lastY = 300.0f;
float fov = 45.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

unsigned int loadTexture(const char* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width = 0, height = 0, nrChannels = 0;
    stbi_set_flip_vertically_on_load(true);

    unsigned char* data =
        stbi_load(path, &width, &height, &nrChannels, 0);

    if (data)
    {
        GLenum format = GL_RGB;

        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexImage2D(
            GL_TEXTURE_2D, 0, format,
            width, height, 0,
            format, GL_UNSIGNED_BYTE, data
        );

        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
            GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load texture: "
            << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

int main()
{
    if (!glfwInit())
    {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(
        SCR_WIDTH, SCR_HEIGHT,
        "Computer Graphics: Lab09.6 - Look Around Pyramid",
        NULL, NULL
    );

    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glEnable(GL_DEPTH_TEST);

    Shader ourShader("vShader.vs", "fShader.glsl");

    float vertices[] = {
        // Front
        -0.5f, -0.5f,  0.5f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f, 1.0f, 0.0f,
         0.0f,  0.5f,  0.0f, 0.5f, 1.0f,

         // Right
          0.5f, -0.5f,  0.5f, 0.0f, 0.0f,
          0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
          0.0f,  0.5f,  0.0f, 0.5f, 1.0f,

          // Back
           0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
          -0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
           0.0f,  0.5f,  0.0f, 0.5f, 1.0f,

           // Left
           -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
           -0.5f, -0.5f,  0.5f, 1.0f, 0.0f,
            0.0f,  0.5f,  0.0f, 0.5f, 1.0f,

            // Base triangle 1
            -0.5f, -0.5f,  0.5f, 0.0f, 0.0f,
             0.5f, -0.5f,  0.5f, 1.0f, 0.0f,
             0.5f, -0.5f, -0.5f, 1.0f, 1.0f,

             // Base triangle 2
              0.5f, -0.5f, -0.5f, 1.0f, 1.0f,
             -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
             -0.5f, -0.5f,  0.5f, 0.0f, 0.0f
    };

    glm::vec3 pyramidPositions[] = {
        glm::vec3(-5.5f,  3.8f, -11.0f),
        glm::vec3(-2.2f,  4.5f, -12.5f),
        glm::vec3(0.8f,  3.2f, -10.0f),
        glm::vec3(3.7f,  4.2f, -11.5f),
        glm::vec3(6.2f,  2.8f, -10.5f),

        glm::vec3(-6.8f,  1.5f, -10.5f),
        glm::vec3(-3.8f,  2.1f, -11.0f),
        glm::vec3(-0.8f,  1.2f,  -9.5f),
        glm::vec3(2.2f,  1.8f, -10.5f),
        glm::vec3(5.5f,  1.0f, -11.0f),

        glm::vec3(-5.0f, -0.8f, -10.0f),
        glm::vec3(-2.0f, -1.5f, -11.5f),
        glm::vec3(1.2f, -0.5f,  -9.8f),
        glm::vec3(4.0f, -1.2f, -11.0f),
        glm::vec3(6.8f, -0.6f, -12.0f),

        glm::vec3(-6.0f, -3.2f, -11.5f),
        glm::vec3(-3.2f, -3.8f, -10.5f),
        glm::vec3(-0.2f, -2.8f, -10.0f),
        glm::vec3(2.8f, -3.5f, -12.0f),
        glm::vec3(5.8f, -2.9f, -10.8f),

        glm::vec3(-7.5f,  2.0f, -14.0f),
        glm::vec3(-4.5f, -2.5f, -13.5f),
        glm::vec3(-1.5f,  3.5f, -14.5f),
        glm::vec3(1.5f, -2.0f, -13.0f),
        glm::vec3(4.5f,  1.5f, -14.0f),
        glm::vec3(-6.0f,  0.0f, -15.0f),
        glm::vec3(-3.0f,  3.0f, -12.0f),
        glm::vec3(0.0f, -3.0f, -14.0f),
        glm::vec3(3.0f,  0.5f, -13.5f),
        glm::vec3(7.0f, -1.0f, -14.5f)
    };

    unsigned int VAO, VBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices),
        vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
        5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    unsigned int texture1 = loadTexture("wall3.jpg");

    ourShader.use();
    ourShader.setInt("texture1", 0);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(1.2f, 0.4f, 1.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);

        ourShader.use();

        glm::mat4 projection = glm::perspective(
            glm::radians(fov),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f, 100.0f
        );
        ourShader.setMat4("projection", projection);

        glm::mat4 view = glm::lookAt(
            cameraPos, cameraPos + cameraFront, cameraUp
        );
        ourShader.setMat4("view", view);

        glBindVertexArray(VAO);

        for (unsigned int i = 0; i < 30; i++)
        {
            glm::mat4 model(1.0f);
            model = glm::translate(model, pyramidPositions[i]);

            float angle = 20.0f * i;
            model = glm::rotate(
                model, glm::radians(angle),
                glm::vec3(1.0f, 0.3f, 0.5f)
            );

            ourShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 18);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteTextures(1, &texture1);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = 2.5f * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;

    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;

    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
        cameraPos -= glm::normalize(
            glm::cross(cameraFront, cameraUp)
        ) * cameraSpeed;

    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
        cameraPos += glm::normalize(
            glm::cross(cameraFront, cameraUp)
        ) * cameraSpeed;
}

void framebuffer_size_callback(
    GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(
    GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) *
        cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) *
        cos(glm::radians(pitch));

    cameraFront = glm::normalize(front);
}

void scroll_callback(
    GLFWwindow* window, double xoffset, double yoffset)
{
    fov -= static_cast<float>(yoffset);

    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f;
}
