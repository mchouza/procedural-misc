// This is *incredibly ugly* "first draft code"

#include <stdlib.h>
#include <SDL.h>
#include <SDL_opengl.h>

#pragma comment(lib, "SDL.lib")
#pragma comment(lib, "SDLmain.lib")
#pragma comment(lib, "opengl32.lib")

void mazeGen(uint8_t** maze, int n)
{
    // an n x n maze is stored as a (2n + 1) x (n + 1) matrix
    // nonzero values in the odd rows of the matrix indicate the presence of
    // of "x" wall segments while the same kind of values in the even rows are
    // associated with the presence of "y" wall segments
    
    // starts disconnected
    int w = n + 1;
    int h = 2 * n + 1;
    for (int i = 0; i < h; i++)
        for (int j = 0; j < w; j++)
            maze[i][j] = 1;
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

    glBegin( GL_LINES );
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