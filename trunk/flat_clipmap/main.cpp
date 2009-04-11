// This is *incredibly ugly* "first draft code"

#define NO_SDL_GLEXT

#include <cmath>
#include <fstream>
#include <map>
#include <string>
#include <vector>
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

    const int LOGN = 7;

	double alpha = 0.0;
	double beta = 0.0;
    double pos[3] = {0.0, 0.0, 1.0};

    double time = 0.0;

    GLuint squareMeshList;
    GLuint holedSquareMeshList;
    
    GLint offsetGLSLVar;
    GLint scaleGLSLVar;
    GLint timeGLSLVar;

	bool keyMap[SDLK_LAST];
}

float clamp(float x, float l, float h)
{
    return (x > h) ? h : ((x < l) ? l: x);
}

void listCurDir(std::vector<std::string>& fileList)
{
    WIN32_FIND_DATA fd;
    HANDLE h = FindFirstFile("*.*", &fd);
    fileList.clear();
    fileList.push_back(std::string(fd.cFileName));
    while (FindNextFile(h, &fd))
        fileList.push_back(std::string(fd.cFileName));
}

void readFileInMemory(const std::string& fn, std::vector<char>& buffer)
{
    std::ifstream ifs(fn.c_str());
    ifs.seekg(0, std::ifstream::end);
    size_t bufSize = ifs.tellg();
    buffer.clear();
    buffer.resize(bufSize);
    ifs.seekg(0);
    ifs.read(&buffer[0], bufSize);
}

void loadShaders(std::map<std::string, GLuint>& vsm, 
                 std::map<std::string, GLuint>& fsm)
{
    vsm.clear();
    fsm.clear();

    std::vector<std::string> fileNames;
    std::vector<char> buffer;
    listCurDir(fileNames);
    for (size_t i = 0; i < fileNames.size(); i++)
    {
        if (fileNames[i].size() < 4)
            continue;
        std::string ext = fileNames[i].substr(fileNames[i].size() - 2);
        if (ext == "vs")
        {
            GLuint vs = glCreateShader(GL_VERTEX_SHADER);
            readFileInMemory(fileNames[i], buffer);
            const char* p = &buffer[0];
            glShaderSource(vs, 1, &p, 0);
            glCompileShader(vs);
            vsm[fileNames[i].substr(0, fileNames[i].size() - 3)] = vs;
        }
        else if (ext == "fs")
        {
            GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
            readFileInMemory(fileNames[i], buffer);
            const char* p = &buffer[0];
            glShaderSource(fs, 1, &p, 0);
            glCompileShader(fs);
            fsm[fileNames[i].substr(0, fileNames[i].size() - 3)] = fs;
        }
    }       
}

void buildSquareMeshList(GLuint& squareMeshList)
{
    unsigned n = 1 << LOGN;
    
    squareMeshList = glGenLists(1);

    float xlo, ylo, xhi, yhi, dx, dy;
    xhi = yhi = 1.0;
    xlo = ylo = -xhi;
    dx = dy = (xhi - xlo) / n;

    glNewList(squareMeshList, GL_COMPILE);
    for (size_t i = 0; i < n; i++)
    {
        glBegin(GL_TRIANGLE_STRIP);
        for (size_t j = 0; j <= n; j++)
        {
            glVertex3f(xlo + j * dx, ylo + i * dy, 0.0f);
            glVertex3f(xlo + j * dx, ylo + (i + 1) * dy, 0.0f);
        }
        glEnd();
    }
    glEndList();
}

void buildHoledSquareMeshList(GLuint& holedSquareMeshList)
{
    unsigned n = 1 << LOGN;
    unsigned m = n / 2;
    
    holedSquareMeshList = glGenLists(1);

    float xlo, ylo, xhi, yhi, dx, dy;
    xhi = yhi = 1.0;
    xlo = ylo = -xhi;
    dx = dy = (xhi - xlo) / n;

    glNewList(holedSquareMeshList, GL_COMPILE);
    for (size_t i = 0; i < n; i++)
    {
        if (i < m / 2 || i >= n / 2 + m / 2)
        {
            glBegin(GL_TRIANGLE_STRIP);
            for (size_t j = 0; j <= n; j++)
            {
                glVertex3f(xlo + j * dx, ylo + i * dy, 0.0f);
                glVertex3f(xlo + j * dx, ylo + (i + 1) * dy, 0.0f);
            }
            glEnd();
        }
    }
    for (size_t j = 0; j < n; j++)
    {
        if (j < m / 2 || j >= n / 2 + m / 2)
        {
            glBegin(GL_TRIANGLE_STRIP);
            for (size_t i = m / 2; i <= n / 2 + m / 2; i++)
            {
                glVertex3f(xlo + j * dx, ylo + i * dy, 0.0f);
                glVertex3f(xlo + (j + 1) * dx, ylo + i * dy, 0.0f);
            }
            glEnd();
        }
    }
    glEndList();
}

void setupClipmap()
{
    GLuint shaderProg = glCreateProgram();
    std::map<std::string, GLuint> vsm, fsm;
    loadShaders(vsm, fsm);
    glAttachShader(shaderProg, vsm["test"]);
    glAttachShader(shaderProg, fsm["test"]);
    glLinkProgram(shaderProg);
    glUseProgram(shaderProg);

    offsetGLSLVar = glGetUniformLocation(shaderProg, "offset");
    scaleGLSLVar = glGetUniformLocation(shaderProg, "scale");
    timeGLSLVar = glGetUniformLocation(shaderProg, "time");

    buildSquareMeshList(squareMeshList);
    buildHoledSquareMeshList(holedSquareMeshList);
}

void setup()
{
    glewInit();
    
    glEnable(GL_DEPTH_TEST);
    //glPolygonMode(GL_FRONT_AND_BACK , GL_LINE);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glViewport(0, 0, 1024, 768);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, 800.0 / 600.0, 0.01, 100.0);

    setupClipmap();
}

void update()
{
	static int lastTime = SDL_GetTicks();
	int newTime = SDL_GetTicks();
	double deltaT = (newTime - lastTime) / 1000.0;
	lastTime = newTime;

    time += deltaT;

	double alphaPrime = 0.0, betaPrime = 0.0, distPrime = 0.0;

	if (keyMap[SDLK_LEFT])
		alphaPrime += 0.5;
	if (keyMap[SDLK_RIGHT])
		alphaPrime -= 0.5;
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
	pos[0] += cos(alpha) * cos(beta) * distPrime * deltaT;
    pos[1] += sin(alpha) * cos(beta) * distPrime * deltaT;
    //pos[2] += sin(beta) * distPrime * deltaT;

	alpha = (alpha > 2 * PI) ? alpha - 2 * PI : alpha;
	alpha = (alpha < -2 * PI) ? alpha +  2 * PI : alpha;
	beta = (beta > PI / 2.1) ? PI / 2.1 : beta;
	beta = (beta < -PI / 2.1) ? -PI / 2.1 : beta;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(
        pos[0], pos[1], pos[2],
		pos[0] + cos(alpha) * cos(beta),
		pos[1] + sin(alpha) * cos(beta),
		pos[2] + sin(beta),
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

    glUniform3f(offsetGLSLVar,(float)pos[0], (float)pos[1], 0.0);
    glUniform1f(timeGLSLVar, (float)time);
    glUniform1f(scaleGLSLVar, 1.0f);
    glCallList(squareMeshList);
    glUniform1f(scaleGLSLVar, 2.0f);
    glCallList(holedSquareMeshList);
    glUniform1f(scaleGLSLVar, 4.0f);
    glCallList(holedSquareMeshList);
    glUniform1f(scaleGLSLVar, 8.0f);
    glCallList(holedSquareMeshList);
    glUniform1f(scaleGLSLVar, 16.0f);
    glCallList(holedSquareMeshList);

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