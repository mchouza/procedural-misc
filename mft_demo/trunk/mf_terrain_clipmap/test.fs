uniform float time;

varying vec4 pos;

void main()
{
    float sh = clamp(pos.z, -1.0, 1.0);

    gl_FragColor = mix(vec4(0.0, 0.0, 0.2, 1.0),
                       vec4(0.2, 0.6, 0.8, 1.0),
                       sh + 1.0) * step(sh, 0.0) +
                   mix(vec4(0.9, 0.9, 0.6, 1.0),
                       vec4(0.1, 0.7, 0.3, 1.0),
                       5.0 * sh) * step(0.0, sh);
}