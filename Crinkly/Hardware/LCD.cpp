#include "LCD.hpp"

#include <print>

std::string vs = R"(
    #version 460 core

    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aTexCoord;
    
    out vec2 TexCoord;

    void main()
    {
        gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
        TexCoord = aTexCoord;
    }
)";

std::string fs = R"(
    #version 460 core
    in vec2 TexCoord;

    out vec4 FragColor;

    uniform sampler2D ourTexture;

    void main()
    {
        FragColor = texture(ourTexture, TexCoord);
    }
)";

LCD::LCD(const std::shared_ptr<Bus>& bus)
{
    m_Bus = bus;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    m_Window = glfwCreateWindow(160 * 3, 144 * 3, "Crinkly", nullptr, nullptr);
    m_WindowDebug = glfwCreateWindow(128 * 3, 192 * 3, "Crinkly Debug", nullptr, nullptr);
    if (m_Window == nullptr || m_WindowDebug == nullptr)
    {
        std::println("Failed to create GLFW window");
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(m_Window);

    if (!gladLoadGLLoader((GLADloadproc)(glfwGetProcAddress)))
    {
        std::println("Failed to initialize GLAD");
        return;
    }

    glViewport(0, 0, 160 * 3, 144 * 3);

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vertexShaderSource = vs.c_str();
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragmentShaderSource = fs.c_str();
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    m_ShaderProgram = glCreateProgram();
    glAttachShader(m_ShaderProgram, vertexShader);
    glAttachShader(m_ShaderProgram, fragmentShader);
    glLinkProgram(m_ShaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    float vertices[] = {
        -1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 0.0f
    };

    U32 indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenTextures(1, &m_Texture);
    glBindTexture(GL_TEXTURE_2D, m_Texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 160, 144, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    glUseProgram(m_ShaderProgram);
    glUniform1i(glGetUniformLocation(m_ShaderProgram, "ourTexture"), 0);

    glfwMakeContextCurrent(m_WindowDebug);
    glViewport(0, 0, 128 * 3, 192 * 3);

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    m_ShaderProgramDebug = glCreateProgram();
    glAttachShader(m_ShaderProgramDebug, vertexShader);
    glAttachShader(m_ShaderProgramDebug, fragmentShader);
    glLinkProgram(m_ShaderProgramDebug);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glGenVertexArrays(1, &m_VAODebug);
    glGenBuffers(1, &m_VBODebug);
    glGenBuffers(1, &m_EBODebug);

    glBindVertexArray(m_VAODebug);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBODebug);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBODebug);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenTextures(1, &m_TextureDebug);
    glBindTexture(GL_TEXTURE_2D, m_TextureDebug);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 192, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    glUseProgram(m_ShaderProgramDebug);
    glUniform1i(glGetUniformLocation(m_ShaderProgramDebug, "ourTexture"), 0);
}

LCD::~LCD()
{
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_EBO);

    glfwTerminate();
}

void LCD::Render()
{
    if (const auto bus = m_Bus.lock())
    {
        if (glfwWindowShouldClose(m_Window) || glfwWindowShouldClose(m_WindowDebug))
        {
            std::println("Window closed");
            this->~LCD();
            system("pause");
            exit(1);
        }

        glfwMakeContextCurrent(m_Window);

        std::vector<U8> m_ViewBuffer;

        for (S32 ty = 17; ty >= 0; ty--)
        {
            for (S32 y = 7; y >= 0; y--)
            {
                for (S32 tx = 0; tx < 20; tx++)
                {
                    for (S32 x = 0; x < 8; x++)
                    {
                        U16 addressOffset = 0x9800 + static_cast<U16>(ty * 32 + tx);
                        U16 tileIndex = bus->Read(addressOffset);
                        U16 address = 0x8000 + tileIndex * 16 + static_cast<U16>(y * 2);

                        U8 leftByte = bus->Read(address);
                        U8 rightByte = bus->Read(address + 1);

                        U8 lsb = (leftByte >> static_cast<U8>(7 - x)) & 0x01;
                        U8 msb = (rightByte >> static_cast<U8>(7 - x)) & 0x01;

                        U8 pixel = static_cast<U8>(msb << 1) | lsb;

                        U8 color = pixel * 85;
                        
                        m_ViewBuffer.push_back(color);
                        m_ViewBuffer.push_back(color);
                        m_ViewBuffer.push_back(color);
                    }
                }
            }
        }

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 160, 144, GL_RGB, GL_UNSIGNED_BYTE, m_ViewBuffer.data());

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_Texture);

        glUseProgram(m_ShaderProgram);
        glBindVertexArray(m_VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(m_Window);
        glfwPollEvents();

        glfwMakeContextCurrent(m_WindowDebug);

        std::vector<U8> m_ViewBufferDebug;

        for (S32 ty = 23; ty >= 0; ty--)
        {
            for (S32 y = 7; y >= 0; y--)
            {
                for (S32 tx = 0; tx < 16; tx++)
                {
                    for (S32 x = 0; x < 8; x++)
                    {
                        if (tx == 0 && x == 0 || (tx == 15 && x == 7))
                        {
                            if (ty < 8)
                            {
                                m_ViewBufferDebug.push_back(255);
                                m_ViewBufferDebug.push_back(0);
                                m_ViewBufferDebug.push_back(0);
                            }
                            else if (ty < 16)
                            {
                                m_ViewBufferDebug.push_back(0);
                                m_ViewBufferDebug.push_back(255);
                                m_ViewBufferDebug.push_back(0);
                            }
                            else
                            {
                                m_ViewBufferDebug.push_back(0);
                                m_ViewBufferDebug.push_back(0);
                                m_ViewBufferDebug.push_back(255);
                            }
                            continue;
                        }
                        else if (y == 7)
                        {
                            if (ty == 7)
                            {
                                m_ViewBufferDebug.push_back(255);
                                m_ViewBufferDebug.push_back(0);
                                m_ViewBufferDebug.push_back(0);
                                continue;
                            }
                            else if (ty == 15)
                            {
                                m_ViewBufferDebug.push_back(0);
                                m_ViewBufferDebug.push_back(255);
                                m_ViewBufferDebug.push_back(0);
                                continue;
                            }
                            else if (ty == 23)
                            {
                                m_ViewBufferDebug.push_back(0);
                                m_ViewBufferDebug.push_back(0);
                                m_ViewBufferDebug.push_back(255);
                                continue;
                            }
                        }
                        else if (y == 0)
                        {
                            if (ty == 0)
                            {
                                m_ViewBufferDebug.push_back(255);
                                m_ViewBufferDebug.push_back(0);
                                m_ViewBufferDebug.push_back(0);
                                continue;
                            }
                            else if (ty == 8)
                            {
                                m_ViewBufferDebug.push_back(0);
                                m_ViewBufferDebug.push_back(255);
                                m_ViewBufferDebug.push_back(0);
                                continue;
                            }
                            else if (ty == 16)
                            {
                                m_ViewBufferDebug.push_back(0);
                                m_ViewBufferDebug.push_back(0);
                                m_ViewBufferDebug.push_back(255);
                                continue;
                            }
                        }

                        U16 byteStride = static_cast<U16>(ty * 16 + tx) * 16;
                        U16 address = 0x8000 + byteStride + static_cast<U16>(y * 2);

                        U8 leftByte = bus->Read(address);
                        U8 rightByte = bus->Read(address + 1);

                        U8 lsb = (leftByte >> static_cast<U8>(7 - x)) & 0x01;
                        U8 msb = (rightByte >> static_cast<U8>(7 - x)) & 0x01;

                        U8 pixel = static_cast<U8>(msb << 1) | lsb;

                        m_ViewBufferDebug.push_back(pixel * 85);
                        m_ViewBufferDebug.push_back(pixel * 85);
                        m_ViewBufferDebug.push_back(pixel * 85);
                    }
                }
            }
        }

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 128, 192, GL_RGB, GL_UNSIGNED_BYTE, m_ViewBufferDebug.data());

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_TextureDebug);

        glUseProgram(m_ShaderProgramDebug);
        glBindVertexArray(m_VAODebug);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(m_WindowDebug);
        glfwPollEvents();
    }
}
