// This is *incredibly ugly* "first draft code"

#define NO_SDL_GLEXT

#include <cmath>
#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>

#pragma comment(lib, "SDL.lib")
#pragma comment(lib, "SDLmain.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "glew32.lib")

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

void colorFromXYZ(float x, float y, float z)
{
    float k = (float)(2.0 * PI * sqrt(3.0));
    float r = 0.9f * (float)(cos(k * z) * cos(k * z)) + 0.5f;
    float g = z;
    float b = z * z;
    glColor3f(r, g, b);
}

void setup()
{
    glewInit();
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    //glPolygonMode(GL_FRONT_AND_BACK , GL_LINE);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glViewport(0, 0, 1024, 768);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, 800.0 / 600.0, 0.01, 100.0);

    glShadeModel(GL_SMOOTH);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLuint shaderProg = glCreateProgram();
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vertShaderSource =  "varying vec3 pos;\n"
                                    "void main()\n"
                                    "{\n"
                                    "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
                                    "    pos = vec3(gl_Vertex);\n"
                                    "}\n";
    glShaderSource(vertShader, 1, &vertShaderSource, 0);
    glCompileShader(vertShader);
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragShaderSource =  "varying vec3 pos;\n"
                                    "void main()\n"
                                    "{\n"
                                    "    gl_FragColor = vec4(sin(pos.z), cos(pos.z), 0.0, 1.0);\n"
                                    "}\n";
    glShaderSource(fragShader, 1, &fragShaderSource, 0);
    glCompileShader(fragShader);
    glAttachShader(shaderProg, vertShader);
    glAttachShader(shaderProg, fragShader);
    glLinkProgram(shaderProg);
    glUseProgram(shaderProg);

    float xlo, ylo, xhi, yhi, dx, dy;
    xhi = yhi = (float)(1.0 / sqrt(3.0));
    xlo = ylo = -xhi;
    dx = dy = (xhi - xlo) / N;

    GLuint sphSegList = glGenLists(1);

    glNewList(sphSegList, GL_COMPILE);
    for (int i = 0; i < N; i++)
    {
        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= N; j++)
        {
            float x = xlo + j * dx;
            float y = ylo + i * dy;
            float z = xhi;
            float norm = sqrt(x * x + y * y + z * z);
            x /= norm;
            y /= norm;
            z /= norm;
            colorFromXYZ(x, y, z);
            glNormal3f(x, y, z);
            glVertex3f(x, y, z);
            x = xlo + j * dx;
            y = ylo + (i + 1) * dy;
            z = xhi;
            norm = sqrt(x * x + y * y + z * z);
            x /= norm;
            y /= norm;
            z /= norm;
            colorFromXYZ(x, y, z);
            glNormal3f(x, y, z);
            glVertex3f(x, y, z);
        }
        glEnd();
    }
    glEndList();

    sphereList = glGenLists(1);

    glNewList(sphereList, GL_COMPILE);
    glMatrixMode(GL_MODELVIEW);
    glCallList(sphSegList);
    glPushMatrix();
    glRotatef(180.0, 1.0, 0.0, 0.0);
    glCallList(sphSegList);
    glPopMatrix();
    glPushMatrix();
    glRotatef(90.0, 1.0, 0.0, 0.0);
    glCallList(sphSegList);
    glRotatef(90.0, 0.0, 1.0, 0.0);
    glCallList(sphSegList);
    glRotatef(90.0, 0.0, 1.0, 0.0);
    glCallList(sphSegList);
    glRotatef(90.0, 0.0, 1.0, 0.0);
    glCallList(sphSegList);
    glPopMatrix();
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
		dist * cos(alpha) * cos(beta),
		dist * sin(alpha) * cos(beta),
		dist * sin(beta),
		0.0, 0.0, 0.0,
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