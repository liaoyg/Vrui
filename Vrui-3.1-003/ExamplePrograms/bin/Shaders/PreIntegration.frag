#version 330
//layout(location = 0)
out vec4 accum;
in vec2 pos;

uniform sampler2D TF;
uniform float deltastep;
uniform float OCSize;


void main(void)
{
    accum = vec4(0.0);
    float d = deltastep;
    float d0 = OCSize;
    float TFSize = float(textureSize(TF, 0));
    float s = pos.x, t = pos.y;
    int steps = int(round(abs(t-s)*TFSize)) + 1;
    float delta = sign(t-s)/TFSize;
    float OCFact = d/(d0*float(steps));

    for(int i=0; i<steps; i++)
    {
        vec4 c = texture(TF, vec2(s,0.0));
        c.a = 1.0 - pow(1.0 - c.a, OCFact);
        c.rgb *= c.a;
        accum += (1.0-accum.a)*c;
        s += delta;
    }
    accum = clamp(accum, vec4(0.0), vec4(1.0));
    //accum = vec4(1.0,0.0,0.0,1.0);
}
