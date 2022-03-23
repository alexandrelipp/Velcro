#include "Application.h"

int main() {
    {
        Application app;
        app.run();
    }
    // cannot be called in app destructor because the app destructor is called before the renderer destructor
    glfwTerminate();
    return 0;
}
