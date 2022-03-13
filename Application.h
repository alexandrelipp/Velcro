//
// Created by alexa on 2021-11-13.
//

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "core/events/Event.h"
#include "core/Codes.h"
#include "core/Render/Renderer.h"
//#include "core/Render/Renderer.h"

#include <spdlog/spdlog.h>
#include <glm/glm.hpp>
#include <memory>


class Application {
public:

    // static methods
    static Application* getApp();

    Application();

    // input polling / events
    bool isKeyPressed(KeyCode code);
    bool isMouseButtonPressed(MouseCode code);
    glm::vec2 getMousePos();
    void onEvent(Event& e);


    void run();
    void close();
    void clean();

    // window
    GLFWwindow* getWindow();
    uint32_t getWindowWidth();
    uint32_t getWindowHeight();
    float getAspectRatio();
   // void setVSync(bool enable);
    bool isVsync();

private:

    bool init();

    GLFWwindow* _window = nullptr;
    //LayerStack _layerStack;

    struct WindowData
    {
        std::string title;
        uint32_t width = 600, height = 600;
        bool VSync;

        std::function<void(Event& e)> eventCallback;
    };

    WindowData _windowData;
    //ImGuiLayer* _imGuiLayer = nullptr;
    //MainLayer* _editorLayer = nullptr;

    static inline Application* _instance = nullptr;

    Renderer _renderer;
};

