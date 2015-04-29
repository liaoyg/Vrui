#version 330

//layout(location = 0)
in vec3 V;
out vec2 pos;

void main(void)
{
    gl_Position = vec4(V, 1.0);
    pos = 0.5*V.xy + vec2(0.5);
    //pos = V.xy;
}
