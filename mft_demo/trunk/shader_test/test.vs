varying vec3 pos;
void main()
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    pos = vec3(gl_Vertex);
}
