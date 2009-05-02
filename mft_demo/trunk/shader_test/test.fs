uniform float time;

varying vec4 pos;

void main()
{
    gl_FragColor = vec4(cos(8.0 * atan(pos.y, pos.x) + time * 2.0),
                        cos(50.0 * (length(pos.xy) * cos(8.0 * atan(pos.y, pos.x)))),
                        cos(50.0 * pos.y) * sin(time),
                        0.0);
}