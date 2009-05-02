#include <GL/glfw.h>

#pragma comment(lib, "glfw.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

namespace
{
    bool quit = false;
}

void GLFWCALL keyCB(int key, int action)
{
    if (key == GLFW_KEY_ESC)
        quit = true;
}

int wcCB()
{
    quit = true;
    return GL_TRUE;
}

int main(void)
{
    glfwInit();

    glfwOpenWindow(1024, 768, 8, 8, 8, 8, 32, 0, GLFW_WINDOW);

    glfwSetKeyCallback(keyCB);
    glfwSetWindowCloseCallback(wcCB);

    while (true)
    {
        if (quit)
        {
            glfwTerminate();
            break;
        }

        glBegin(GL_LINE_STRIP);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(1.0f, 0.0f);
        glEnd();

        glfwSwapBuffers();
    }
}