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

    const int N = 128;

	double alpha = 0.0;
	double beta = 0.0;
	double dist = 2.0;

    GLuint terrainList;

	bool keyMap[SDLK_LAST];

    int perm[256]= {151,160,137,91,90,15,
      131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
      190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
      88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
      77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
      102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
      135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
      5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
      223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
      129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
      251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
      49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
      138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180};

    /* These are Ken Perlin's proposed gradients for 3D noise. I kept them for
       better consistency with the reference implementation, but there is really
       no need to pad this to 16 gradients for this particular implementation.
       If only the "proper" first 12 gradients are used, they can be extracted
       from the grad4[][] array: grad3[i][j] == grad4[i*2][j], 0<=i<=11, j=0,1,2
    */
    int grad3[16][3] = {{0,1,1},{0,1,-1},{0,-1,1},{0,-1,-1},
                       {1,0,1},{1,0,-1},{-1,0,1},{-1,0,-1},
                       {1,1,0},{1,-1,0},{-1,1,0},{-1,-1,0}, // 12 cube edges
                       {1,0,-1},{-1,0,-1},{0,-1,1},{0,1,1}}; // 4 more to make 16
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

/*** Shamelessly stolen from Stefan Gustavson (stegu@itn.liu.se) 2004, 2005 */

/*
 * initPermTexture(GLuint *texID) - create and load a 2D texture for
 * a combined index permutation and gradient lookup table.
 * This texture is used for 2D and 3D noise, both classic and simplex.
 */
void initPermTexture(GLuint *texID)
{
  char *pixels;
  int i,j;
  
  glGenTextures(1, texID); // Generate a unique texture ID
  glBindTexture(GL_TEXTURE_2D, *texID); // Bind the texture to texture unit 0

  pixels = (char*)malloc( 256*256*4 );
  for(i = 0; i<256; i++)
    for(j = 0; j<256; j++) {
      int offset = (i*256+j)*4;
      char value = perm[(j+perm[i]) & 0xFF];
      pixels[offset] = grad3[value & 0x0F][0] * 64 + 64;   // Gradient x
      pixels[offset+1] = grad3[value & 0x0F][1] * 64 + 64; // Gradient y
      pixels[offset+2] = grad3[value & 0x0F][2] * 64 + 64; // Gradient z
      pixels[offset+3] = value;                     // Permuted index
    }
  
  // GLFW texture loading functions won't work here - we need GL_NEAREST lookup.
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
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
    std::map<std::string, GLuint> vsm, fsm;
    loadShaders(vsm, fsm);
    glAttachShader(shaderProg, vsm["noise"]);
    glAttachShader(shaderProg, fsm["noise"]);
    glLinkProgram(shaderProg);
    glUseProgram(shaderProg);

    terrainList = glGenLists(1);

    float xlo, ylo, xhi, yhi, dx, dy;
    xhi = yhi = 1.0;
    xlo = ylo = -xhi;
    dx = dy = (xhi - xlo) / N;

    glNewList(terrainList, GL_COMPILE);
    for (int i = 0; i < N; i++)
    {
        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= N; j++)
        {
            glNormal3f(0.0f, 0.0f, 1.0f);
            glVertex3f(xlo + j * dx, ylo + i * dy, 0.0f);
            glNormal3f(0.0f, 0.0f, 1.0f);
            glVertex3f(xlo + j * dx, ylo + (i + 1) * dy, 0.0f);
        }
        glEnd();
    }
    glEndList();

    GLuint dummy;
    initPermTexture(&dummy);
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

    glCallList(terrainList);

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