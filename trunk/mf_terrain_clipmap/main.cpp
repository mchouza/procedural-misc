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

    const int LOGN = 8;

	double alpha = PI;
	double beta = 0.0;
    double pos[3] = {0.0, 0.0, 0.5};

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

  free(pixels);
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

    GLuint dummy;
    initPermTexture(&dummy);
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
        fps = 0.01 * (1.0 / deltaT) + 0.99 * fps;

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

    glClearColor(0.4, 0.8, 1.0, 1.0);    
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
    glUniform1f(scaleGLSLVar, 32.0f);
    glCallList(holedSquareMeshList);
    glCallList(adapterMeshList);
    glUniform1f(scaleGLSLVar, 64.0f);
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