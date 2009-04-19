uniform float time;

varying vec4 pos;

void main()
{
    float sh = (pos.z - 0.02) * 10.0;
    gl_FragColor = vec4(clamp(sh * 2.0 - 1.0, 0.0, 0.7),
                        clamp(sh * 10.0, 0.0, 0.7),
                        clamp(-sh * 30.0, 0.0, 1.0) + clamp(sh * 2.0 - 1.0, 0.0, 0.7),
                        1.0);
}