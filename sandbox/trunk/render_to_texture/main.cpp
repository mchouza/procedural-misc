#include <cstring>
#include <GL/glfw.h>

//#define RTT_WITH_FB

#pragma comment(lib, "glfw.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

namespace
{
    bool quit = false;

    GLuint rtex;
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

void renderToTextureOldWay()
{
    GLint prevBoundTex;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevBoundTex);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glViewport(0, 0, 128, 128);
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_LINES);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();
    
    glBindTexture(GL_TEXTURE_2D, prevBoundTex);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 0, 0, 128, 128, 0);
}

void renderToTextureWithFB()
{
    // FIXME: IMPLEMENT!
}

void init()
{
    void* buffer = new char[128*128*4];
    memset(buffer, 0, 128*128*4);

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &rtex);
    glBindTexture(GL_TEXTURE_2D, rtex);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    delete[] buffer;
#ifdef RTT_WITH_FB
    renderToTextureWithFB();
#else
    renderToTextureOldWay();
#endif

    glViewport(0, 0, 1024, 768);
}

void draw()
{
    static int frame = 0;
    frame++;
    float offset = (frame % 100) / 100.0f;
    
    glClear(GL_COLOR_BUFFER_BIT);

    glBegin(GL_QUADS);
    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            glTexCoord2f(i / 16.0f, j / 16.0f + offset);
            glVertex3f(i / 32.0f, j / 32.0f, 1.0f);
            glTexCoord2f((i + 1) / 16.0f, j / 16.0f + offset);
            glVertex3f((i + 1) / 32.0f, j / 32.0f, 1.0f);
            glTexCoord2f((i + 1) / 16.0f, (j + 1) / 16.0f + offset);
            glVertex3f((i + 1) / 32.0f, (j + 1) / 32.0f, 1.0f);
            glTexCoord2f(i / 16.0f, (j + 1) / 16.0f + offset);
            glVertex3f(i / 32.0f, (j + 1) / 32.0f, 1.0f);
        }
    }
    glEnd();
}

int main(void)
{
    glfwInit();

    glfwOpenWindow(1024, 768, 8, 8, 8, 8, 32, 0, GLFW_WINDOW);

    glfwSetKeyCallback(keyCB);
    glfwSetWindowCloseCallback(wcCB);

    init();

    while (true)
    {
        if (quit)
        {
            glfwTerminate();
            break;
        }

        draw();

        glfwSwapBuffers();
    }
}