// This is *incredibly ugly* "first draft code"

#include <cmath>
#include <SDL.h>
#include <SDL_opengl.h>

extern "C" double noise2(double *);

namespace
{
	const double PI = 3.1415926535897932384626433832795;

	double alpha = 0.0;
	double beta = 0.0;
	double dist = 2.0;

    GLuint terrainList;

	bool keyMap[SDLK_LAST];
}

float clamp(float x, float l, float h)
{
    return (x > h) ? h : ((x < l) ? l: x);
}

float mfNoise(float x, float y)
{
    float ret = 0;
    float amp = 1.0f;
    float signal;
    double r[2];
    float weight = 1.0;

    r[0] = x;
    r[1] = y;

    for (int i = 0; i < 5; i++)
    {
        ret += (signal = weight * amp * (float)noise2(r));
        amp /= 1.5f;
        r[0] *= 2.0f;
        r[1] *= 2.0f;
        weight = clamp(signal / 0.1f, 0.0f, 1.0f);
    }

    return ret;
}

void fillHeightMap(float* hm)
{
    for (int i = 0; i < 128 * 128; i++)
    {
        float x, y;
        x = (8.0f * (float)(i % 128)) / 128;
        y = (8.0f * (float)(i / 128)) / 128;
        hm[i] = mfNoise(x, y) / 18.0f;
    }
}

void fillNormalMap(float* nm, float* hm)
{
    for (int i = 0; i < 128; i++)
    {
        for (int j = 0; j < 128; j++)
        {
            float val = hm[i * 128 + j];
            float nextX = (j < 128 - 1) ?
                          hm[i * 128 + (j + 1)] :
                          2 * val - hm[i * 128 + (j - 1)];
            float nextY = (i < 128 - 1) ?
                          hm[(i + 1) * 128 + j] :
                          2 * val - hm[(i - 1) * 128 + j];
            nm[3 * (128 * i + j) + 0] = -128.0f * (nextX - val);
            nm[3 * (128 * i + j) + 1] = -128.0f * (nextY - val);
            nm[3 * (128 * i + j) + 2] = 1.0f;
        }
    }
}

void setup()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
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

    static float hm[128*128];
    static float nm[128*128*3];
    
    fillHeightMap(hm);
    fillNormalMap(nm, hm);

    terrainList = glGenLists(1);

    glNewList(terrainList, GL_COMPILE); 
    for (int i = 0; i < 128 - 1; i++)
    {
        glBegin(GL_TRIANGLE_STRIP);
        glNormal3fv(&nm[((i + 1) * 128 + 0) * 3 + 0]);
        glVertex3f(0.0f, (i + 1) / 128.0f, hm[(i + 1) * 128]);
        glNormal3fv(&nm[(i * 128 + 0) * 3 + 0]);
        glVertex3f(0.0f, i / 128.0f, hm[i * 128]);
        for (int j = 1; j < 128; j++)
        {
            glNormal3fv(&nm[((i + 1) * 128 + j) * 3 + 0]);
            glVertex3f(j / 128.0f, (i + 1) / 128.0f, hm[(i + 1) * 128 + j]);
            glNormal3fv(&nm[(i * 128 + j) * 3 + 0]);
            glVertex3f(j / 128.0f, i / 128.0f, hm[i * 128 + j]);
        }
        glEnd();
    }
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
	
	if (e.type == SDL_KEYDOWN)
		keyMap[e.key.keysym.sym] = true;
	else if (e.type == SDL_KEYUP)
		keyMap[e.key.keysym.sym] = false;

	return true;
}

void draw()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glCallList(terrainList);

    SDL_GL_SwapBuffers();
}

int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_SetVideoMode(1024, 768, 32, SDL_OPENGL | SDL_FULLSCREEN);
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