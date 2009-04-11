varying vec4 pos;
void main()
{
    gl_FragColor = vec4(cos(8.0 * atan(pos.y, pos.x)),
                        cos(50.0 * (length(pos.xy) * cos(8.0 * atan(pos.y, pos.x)))),
                        cos(50.0 * pos.y),
                        0.0);
}
