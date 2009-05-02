uniform vec3 offset;
uniform float scale;
uniform float time;

varying vec4 pos;

void main()
{
    pos = vec4(scale * gl_Vertex.xyz, 1.0);
    pos += vec4(offset.xyz, 0.0);
    pos += vec4(0.0, 
                0.0,
                0.1 * (sin(5.0 * pos.x) + cos(8.0 * pos.y)) +
                0.3 * (sin(0.4 * pos.x) + cos(0.2 * pos.y)),
                0.0);
    gl_Position = gl_ModelViewProjectionMatrix * pos;
}
