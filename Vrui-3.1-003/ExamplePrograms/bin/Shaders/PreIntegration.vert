attribute vec3 vertex;

varying vec2 pos;

void main(void)
{
    gl_Position = vec4(vertex, 1.0);
    pos = 0.5*vertex.xy + vec2(0.5);
}
