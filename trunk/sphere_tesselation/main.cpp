// This is *incredibly ugly* "first draft code"

#include <cmath>
#include <SDL.h>
#include <SDL_opengl.h>

namespace
{
	const double PI = 3.1415926535897932384626433832795;

    const int N = 128;

	double alpha = 0.0;
	double beta = 0.0;
	double dist = 2.0;

    GLuint sphereList;

	bool keyMap[SDLK_LAST];
}

float clamp(float x, float l, float h)
{
    return (x > h) ? h : ((x < l) ? l: x);
}

void setup()
{
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    //glPolygonMode(GL_FRONT_AND_BACK , GL_LINE);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glViewport(0, 0, 1024, 768);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, 800.0 / 600.0, 0.01, 100.0);

    float lightPos[4] = {0.0f, 1.0f, 1.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glShadeModel(GL_SMOOTH);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    sphereList = glGenLists(1);

    glNewList(sphereList, GL_COMPILE); 
    glEndList();
}

void update()
{
	static int lastTime = SDL_GetTicks();
	int newTime = SDL_GetTicks();
	double deltaT = (newTime - lastTime) / 1000.0;
	lastTime = newTime;

	double alphaPrime = 0.0, betaPrime = 0.0, distPrime = 0.0;

	if (keyMap[SDLK_LEFT])
		alphaPrime -= 0.5;
	if (keyMap[SDLK_RIGHT])
		alphaPrime += 0.5;
	if (keyMap[SDLK_DOWN])
		betaPrime -= 0.5;
	if (keyMap[SDLK_UP])
		betaPrime += 0.5;
	if (keyMap[SDLK_PAGEDOWN])
		distPrime += 2.5;
	if (keyMap[SDLK_PAGEUP])
		distPrime -= 2.5;

	alpha += deltaT * alphaPrime;
	beta += deltaT * betaPrime;
	dist += deltaT * distPrime;

	alpha = (alpha > 2 * PI) ? alpha - 2 * PI : alpha;
	alpha = (alpha < -2 * PI) ? alpha +  2 * PI : alpha;
	beta = (beta > 1.0) ? 1.0 : beta;
	beta = (beta < -1.0) ? -1.0 : beta;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(
		dist * cos(alpha) * cos(beta) + 0.5,
		dist * sin(alpha) * cos(beta) + 0.5,
		dist * sin(beta),
		0.5, 0.5, 0.0,
		0.0, 0.0, 1.0);
}

bool handleEvent(const SDL_Event& e)
{	
	if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
		return false;

    if (e.type == SDL_QUIT)
        return false;
	
	if (e.type == SDL_KEYDOWN)
		keyMap[e.key.keysym.sym] = true;
	else if (e.type == SDL_KEYUP)
		keyMap[e.key.keysym.sym] = false;

	return true;
}

void draw()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glCallList(sphereList);

    SDL_GL_SwapBuffers();
}

int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_SetVideoMode(1024, 768, 32, SDL_OPENGL);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	setup();

	SDL_Event e;
	while (true)
	{
		if (SDL_PollEvent(&e))
			if (!handleEvent(e))
				break;

		update();
		draw();
	}

	SDL_Quit();
	return 0;
}