varying vec4 pos;
void main()
{
    pos = vec4(gl_Vertex);
    pos += vec4(0.0, 
                0.0,
                0.1 * (sin(5.0 * pos.x) + cos(8.0 * pos.y)),
                0.0);
    gl_Position = gl_ModelViewProjectionMatrix * pos;
}
