//
// Created by alexa on 2021-11-13.
//

#include "Application.h"
#include "core/events/MouseEvent.h"
#include "core/events/KeyEvent.h"


Application::Application() {
    if (_instance != nullptr){
        throw std::runtime_error("There is already an instance of application");
    }
    // bind the window event callback to this->onEvent
    _windowData.eventCallback = [this](Event& e) { onEvent(e); };
    if (!init())
        throw std::runtime_error("Failed to init the application");
    _instance = this;

    _renderer.init();

    //_imGuiLayer = new ImGuiLayer();
    //_layerStack.addLayer(_imGuiLayer);
    //_editorLayer = new MainLayer();
    //_layerStack.addLayer(_editorLayer);
}

Application* Application::getApp(){
    return _instance;
}

GLFWwindow* Application::getWindow(){
    return _window;
}

bool Application::isKeyPressed(KeyCode code){
    return glfwGetKey(_window, (int)code) == GLFW_PRESS;
}

bool Application::isMouseButtonPressed(MouseCode code){
    return glfwGetMouseButton(_window, (int)code) == GLFW_PRESS;
}

glm::vec2 Application::getMousePos(){
    double x, y;
    glfwGetCursorPos(_window, &x, &y);
    return {x, y};
}

bool Application::init(){

    if (!glfwInit()){
        SPDLOG_ERROR("Failed to init glfw");
        return false;
    }

    //no openGL nor openGL es
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // complex for now
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // create window and make sure it's successful
    _window = glfwCreateWindow(_windowData.width, _windowData.height, "Reborn", nullptr, nullptr);
    if (_window == nullptr)
        return false;



    // data pointer used by glfw
    glfwSetWindowUserPointer(_window, &_windowData);

    // set resize window callback
//    glfwSetFramebufferSizeCallback(_window, [](GLFWwindow* window, int width, int height) {
//        WindowData* data = (WindowData*)glfwGetWindowUserPointer(window);
//        data->width = width;
//        data->height = height;
//        glViewport(0, 0, width, height);
//    });

    glfwSetMouseButtonCallback(_window, [](GLFWwindow* window, int button, int action, int mods){
        WindowData* data = (WindowData*)glfwGetWindowUserPointer(window);
        switch (action) {
            case GLFW_PRESS:{
                MouseButtonPressedEvent e((MouseCode)button);
                data->eventCallback(e);
                return;
            }
            case GLFW_RELEASE:{
                MouseButtonReleasedEvent e((MouseCode)button);
                data->eventCallback(e);
                return;
            }
            default:
                SPDLOG_ERROR("Unknown action of type {}", action);
        }
    });

    glfwSetScrollCallback(_window, [](GLFWwindow* window, double xOffset, double yOffset){
        WindowData* data = (WindowData*)glfwGetWindowUserPointer(window);
        MouseScrolledEvent e(xOffset, yOffset);
        data->eventCallback(e);
    });

    glfwSetCursorPosCallback(_window, [](GLFWwindow* window, double xpos, double ypos){
        WindowData* data = (WindowData*)glfwGetWindowUserPointer(window);
        MouseMovedEvent e(xpos, ypos);
        data->eventCallback(e);
    });

    glfwSetKeyCallback(_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        WindowData *data = (WindowData *) glfwGetWindowUserPointer(window);
        switch (action) {
            case GLFW_PRESS: {
                KeyPressedEvent e((KeyCode) key, 0);
                data->eventCallback(e);
                return;
            }
            case GLFW_RELEASE: {
                KeyReleasedEvent e((KeyCode) key);
                data->eventCallback(e);
                return;
            }
            case GLFW_REPEAT: {
                KeyPressedEvent e((KeyCode) key, 1);
                data->eventCallback(e);
                return;
            }
            default:
                SPDLOG_ERROR("Unknown action of type {}", action);
        }
    });

    glfwSetCharCallback(_window, [](GLFWwindow* window, unsigned int character){
        WindowData* data = (WindowData*) glfwGetWindowUserPointer(window);
        KeyTypedEvent e((KeyCode)character);
        data->eventCallback(e);
    });

    glfwSetErrorCallback([](int error_code, const char* description){
        SPDLOG_ERROR("GLFW error {}", description);
    });

    // TODO : renable when pipeline can support window resize!
//    glfwSetWindowSizeCallback(_window, [](GLFWwindow* window, int width, int height){
//        WindowData* data = (WindowData*) glfwGetWindowUserPointer(window);
//        data->width = width;
//        data->height = height;
//        WindowResizeEvent e(width, height);
//        data->eventCallback(e);
//    });

    //setVSync(true);
    return true;
}

void Application::run() {
    double currentFrame = 0, lastFrame = 0;
    while (!glfwWindowShouldClose(_window))
    {
        // update layer stack with delta time
//        currentFrame = glfwGetTime();
//        _layerStack.onUpdate(currentFrame - lastFrame);
//        lastFrame = currentFrame;
//
//        if constexpr (Application::GRAPHICS_API == GraphicsAPI::OPEN_GL){
//            _imGuiLayer->begin();
//            _layerStack.onImGuiRender();
//            _imGuiLayer->end();
//        }
        _renderer.draw();
        glfwPollEvents();
    }
}

void Application::close() {
    glfwSetWindowShouldClose(_window, true);
}

void Application::onEvent(Event &e) {
    //SPDLOG_INFO("event {}", e.toString());
    //_layerStack.onEvent(e);
    switch (e.getType()) {
        case Event::Type::KEY_PRESSED:{
            KeyPressedEvent keyPressedEvent = *(KeyPressedEvent*)&e;
            if (keyPressedEvent.getKeyCode() == KeyCode::Escape)
                close();
        }
            return;
        default:
            return;
    }
}

Renderer* Application::getRenderer() {
    return &_renderer;
}


uint32_t Application::getWindowWidth(){
    return _windowData.width;
}

uint32_t Application::getWindowHeight(){
    return _windowData.height;
}


float Application::getAspectRatio() {
    return _windowData.width/(float)_windowData.height;
}

void Application::clean(){
    //_layerStack.shutDown();
    glfwDestroyWindow(_window);
    _window = nullptr;

    glfwTerminate();
}

//void Application::setVSync(bool enable) {
//    glfwSwapInterval(enable ? 1 : 0);
//    _windowData.VSync = enable;
//}

bool Application::isVsync() {
   return _windowData.VSync;
}