varying vec3 pos;
void main()
{
    gl_FragColor = vec4(cos(5.0 * pos.z),
                        cos(8.0 * atan(pos.y, pos.x)),
                        cos(10.0 * pos.z),
                        0.0);
}
