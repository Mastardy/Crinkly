#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Bus.hpp"

class LCD
{
public:
    LCD(const std::shared_ptr<Bus>& bus);
    ~LCD();

    void Render();

private:
    GLFWwindow* m_Window;
    GLFWwindow* m_WindowDebug;
    
    std::weak_ptr<Bus> m_Bus;

    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    unsigned int m_EBO = 0;
    unsigned int m_ShaderProgram = 0;
    unsigned int m_Texture = 0;

    unsigned int m_VAODebug = 0;
    unsigned int m_VBODebug = 0;
    unsigned int m_EBODebug = 0;
    unsigned int m_ShaderProgramDebug = 0;
    unsigned int m_TextureDebug = 0;
};
