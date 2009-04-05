// This is *incredibly ugly* "first draft code"

#include <stdlib.h>
#include <SDL.h>
#include <SDL_opengl.h>

GLuint mfTexID;

extern "C" double noise2(double *);

float mfNoise(float x, float y)
{
    float ret = 0;
    float amp = 1.0f;
    double r[2];

    r[0] = x;
    r[1] = y;

    for (int i = 0; i < 5; i++)
    {
        ret += (float)(amp * noise2(r));
        amp /= 3.0f;
        r[0] *= 2.0f;
        r[1] *= 2.0f;
    }

    return ret;
}

void fillHeightMap(uint32_t* hm)
{
    for (int i = 0; i < 512 * 512; i++)
    {
        float x, y;
        x = (8.0f * (float)(i % 512)) / 512;
        y = (8.0f * (float)(i / 512)) / 512;
        int h = (int)(127.5 * (mfNoise(x, y) + 1.0));
        hm[i] = h | h << 8 | h << 16;
    }
}

bool initVideo()
{
    if (SDL_Init(SDL_INIT_VIDEO))
        return false;
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Surface* screen = SDL_SetVideoMode(640, 480, 32, SDL_OPENGL);
    if (!screen)
        return false;

    return true;
}

bool initGL()
{
    glEnable(GL_TEXTURE_2D);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glViewport(0, 0, 640, 480);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, 640, 480, 0.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    return true;
}

bool initWorld()
{
    glGenTextures(1, &mfTexID);
    glBindTexture(GL_TEXTURE_2D, mfTexID);

    uint32_t* pTexData = new uint32_t[512 * 512];
    fillHeightMap(pTexData);

    glTexImage2D(GL_TEXTURE_2D, 0, 4, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, 
        pTexData);

    delete[] pTexData;

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);

    glEnable(GL_TEXTURE_2D);

    if (glGetError() != GL_NO_ERROR)
        return false;

    return true;
}

bool detectQuitRequest()
{
    SDL_Event ev;
    if (!SDL_PollEvent(&ev))
        return false;
    return ev.type == SDL_QUIT;
}

bool drawWorld()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, mfTexID);

    glBegin( GL_QUADS );
        glColor3f(1, 1, 1);
        glTexCoord2i(0, 0);
        glVertex2i(0, 0);
        glTexCoord2i(1, 0);
        glVertex2i(500, 0);
        glTexCoord2i(1, 1);
        glVertex2i(500, 500);
        glTexCoord2i(0, 1);
        glVertex2i(0, 500);
    glEnd();

    SDL_GL_SwapBuffers();

    if (glGetError() != GL_NO_ERROR)
        return false;

    return true;
}

void finishVideo()
{
    SDL_Quit();
}

int main(int argc, char* argv[])
{
    if (!initVideo())
        return 1;
    if (!initGL())
        return 1;
    if (!initWorld())
        return 1;

    while (!detectQuitRequest())
        if (!drawWorld())
            break;

    finishVideo();

    return 0;
}