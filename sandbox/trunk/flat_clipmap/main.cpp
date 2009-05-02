// This is *incredibly ugly* "first draft code"

#define NO_SDL_GLEXT

#include <cmath>
#include <fstream>
#include <map>
#include <string>
#include <sstream>
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

	double alpha = PI;
	double beta = 0.0;
    double pos[3] = {0.0, 0.0, 1.0};

    double time = 0.0;
    double fps = 0.0;

    GLuint squareMeshVB;

    GLuint adapterMeshList;
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

void buildAdapterMeshList(GLuint& adapterMeshList)
{
    unsigned n = 1 << LOGN;
    
    adapterMeshList = glGenLists(1);

    glBindBuffer(GL_ARRAY_BUFFER, squareMeshVB);
    GLuint adMeshEB;
    glGenBuffers(1, &adMeshEB);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, adMeshEB);

    GLuint* pEdgeData = new GLuint[((n - 2) / 2 + 1) * 4 * 3];
    GLuint* p = pEdgeData;

    for (size_t i = 0; i <= n - 2; i += 2)
    {
        // top
        *p++ = i + 1;
        *p++ = i;
        *p++ = i + 2;
        // bottom
        *p++ = i + (n + 1) * n;
        *p++ = i + 1 + (n + 1) * n;
        *p++ = i + 2 + (n + 1) * n;
        // left
        *p++ = i * (n + 1);
        *p++ = (i + 1) * (n + 1);
        *p++ = (i + 2) * (n + 1);
        // right
        *p++ = (i + 1) * (n + 1) + n;
        *p++ = i * (n + 1) + n;
        *p++ = (i + 2) * (n + 1) + n;
    }

    glVertexPointer(4, GL_FLOAT, 0, 0);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (p - pEdgeData) * sizeof(GLuint), pEdgeData, GL_STATIC_DRAW);
    delete[] pEdgeData;

    glNewList(adapterMeshList, GL_COMPILE);
    glDrawElements(GL_TRIANGLES, p - pEdgeData, GL_UNSIGNED_INT, 0);
    glEndList();
}

void buildSquareMeshList(GLuint& squareMeshList)
{
    unsigned n = 1 << LOGN;
    
    squareMeshList = glGenLists(1);

    glGenBuffers(1, &squareMeshVB);
    glBindBuffer(GL_ARRAY_BUFFER, squareMeshVB);
    GLuint sqMeshEB;
    glGenBuffers(1, &sqMeshEB);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sqMeshEB);

    float xlo, ylo, xhi, yhi, dx, dy;
    xhi = yhi = 1.0;
    xlo = ylo = -xhi;
    dx = dy = (xhi - xlo) / n;

    float* pVertexData = new float[(n + 1) * (n + 1) * 4];
    for (size_t i = 0; i <= n; i++)
    {
        for (size_t j = 0; j <= n; j++)
        {
            pVertexData[4 * ((n + 1) * i + j) + 0] = xlo + dx * j;
            pVertexData[4 * ((n + 1) * i + j) + 1] = ylo + dy * i;
            pVertexData[4 * ((n + 1) * i + j) + 2] = 0.0f;
            pVertexData[4 * ((n + 1) * i + j) + 3] = 1.0f;
        }
    }

    GLuint* pEdgeData = new GLuint[n * (n + 1) * 2 + n];
    GLuint* p = pEdgeData;
    for (size_t i = 0; i < n; i++)
    {
        for (size_t j = 0; j <= n; j++)
        {
            *p++ = (i % 2) ? (n + 1) * i + (n - j) : (n + 1) * (i + 1) + j;
            *p++ = (i % 2) ? (n + 1) * (i + 1) + (n - j) : (n + 1) * i + j;
        }
    }

    glBufferData(GL_ARRAY_BUFFER, (n + 1) * (n + 1) * 4 * sizeof(GLfloat), pVertexData, GL_STATIC_DRAW);
    glVertexPointer(4, GL_FLOAT, 0, 0);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (n * (n + 1) * 2 + n) * sizeof(GLuint), pEdgeData, GL_STATIC_DRAW);
    delete[] pVertexData;
    delete[] pEdgeData;

    glNewList(squareMeshList, GL_COMPILE);
    glDrawElements(GL_TRIANGLE_STRIP, n * (n + 1) * 2, GL_UNSIGNED_INT, 0);
    glEndList();
}

void buildHoledSquareMeshList(GLuint& holedSquareMeshList)
{
    unsigned n = 1 << LOGN;
    unsigned m = n / 4;
    
    holedSquareMeshList = glGenLists(1);

    glBindBuffer(GL_ARRAY_BUFFER, squareMeshVB);
    GLuint hsqMeshEB;
    glGenBuffers(1, &hsqMeshEB);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hsqMeshEB);

    // I'm too lazy to do this calculation properly ( :-D ), so this is a 
    // (manual) regression: 
    // N = 128 Size = 24705
    // N = 64 Size = 6209
    // N = 32 Size = 1569

    // Size = 2 * (N^2 - (N/2)^2) + N + 1
    // Size = 2 * (3 N^2 / 4) + N + 1 
    // Size = 3 N^2 / 2 + N + 1

    GLuint* pEdgeData = new GLuint[3 * n * n / 2 + n + 1];
    GLuint* p = pEdgeData;
    GLuint offset = n + 1;
    GLuint intVI = offset * m + m;
    GLuint extVI = intVI - offset;
    GLuint spiralSegLen = m * 2;
    // start
    *p++ = intVI;
    *p++ = extVI;
    extVI++;
    intVI++;
    for (unsigned spiral = 0; spiral < m; spiral++)
    {
        // upper segment
        for (unsigned i = 0; i < spiralSegLen; i++)
        {
            *p++ = intVI;
            *p++ = extVI;
            extVI++;
            intVI++;
        }
        // corner
        *p++ = extVI;
        extVI += offset;
        intVI--;
        // right segment
        for (unsigned i = 0; i <= spiralSegLen; i++)
        {
            *p++ = extVI;
            *p++ = intVI;
            extVI += offset;
            intVI += offset;
        }
        // corner
        *p++ = extVI;
        extVI--;
        intVI -= offset;
        // bottom segment
        for (unsigned i = 0; i <= spiralSegLen; i++)
        {
            *p++ = intVI;
            *p++ = extVI;
            extVI--;
            intVI--;
        }
        // corner
        *p++ = extVI;
        extVI -= offset;
        intVI++;
        // left segment (one module longer)
        for (unsigned i = 0; i <= spiralSegLen + 1; i++)
        {
            *p++ = extVI;
            *p++ = intVI;
            extVI -= offset;
            intVI -= offset;
        }
        // skips the last corner, as it's not present
        if (spiral == m - 1)
            break;
        // corner
        *p++ = extVI;
        extVI++;
        intVI += offset;
        // now spiral segments are longer
        spiralSegLen += 2;
    }

    glVertexPointer(4, GL_FLOAT, 0, 0);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (3 * n * n / 2 + n + 1) * sizeof(GLuint), pEdgeData, GL_STATIC_DRAW);
    delete[] pEdgeData;

    glNewList(holedSquareMeshList, GL_COMPILE);
    glDrawElements(GL_TRIANGLE_STRIP, 3 * n * n / 2 + n + 1, GL_UNSIGNED_INT, 0);
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
    buildAdapterMeshList(adapterMeshList);
    buildHoledSquareMeshList(holedSquareMeshList);
}

void setup()
{
    glewInit();
    
    glEnable(GL_DEPTH_TEST);
    //glPolygonMode(GL_FRONT_AND_BACK , GL_LINE);
    glEnable(GL_CULL_FACE);

    glEnableClientState(GL_VERTEX_ARRAY);

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
    if (deltaT > 0)
        fps = 0.1 * (1.0 / deltaT) + 0.9 * fps;

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
#if 0
	pos[0] += cos(alpha) * cos(beta) * distPrime * deltaT;
    pos[1] += sin(alpha) * cos(beta) * distPrime * deltaT;
    pos[2] += sin(beta) * distPrime * deltaT;
#else
	pos[0] += cos(alpha) * distPrime * deltaT;
    pos[1] += sin(alpha) * distPrime * deltaT;
#endif

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
    std::ostringstream s;
    s << fps;
    SDL_WM_SetCaption(s.str().c_str(), 0);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUniform3f(offsetGLSLVar,(float)pos[0], (float)pos[1], 0.0);
    glUniform1f(timeGLSLVar, (float)time);
    glUniform1f(scaleGLSLVar, 1.0f);
    glCallList(squareMeshList);
    glCallList(adapterMeshList);
    glUniform1f(scaleGLSLVar, 2.0f);
    glCallList(holedSquareMeshList);
    glCallList(adapterMeshList);
    glUniform1f(scaleGLSLVar, 4.0f);
    glCallList(holedSquareMeshList);
    glCallList(adapterMeshList);
    glUniform1f(scaleGLSLVar, 8.0f);
    glCallList(holedSquareMeshList);
    glCallList(adapterMeshList);
    glUniform1f(scaleGLSLVar, 16.0f);
    glCallList(holedSquareMeshList);
    glCallList(adapterMeshList);

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