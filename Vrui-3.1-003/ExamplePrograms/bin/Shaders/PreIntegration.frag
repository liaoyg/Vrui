uniform sampler2D transferFTex;
uniform float delta_step;
uniform float OCSize;

varying vec2 pos;

void main(void)
{
    vec4 accum = vec4(0.0);
    float d = delta_step;
    float correction_delta = OCSize;
    float size_TF = float(textureSize(transferFTex,0));
    float s = pos.x, t = pos.y;
    int steps = ceil(abs(t-s)*size_TF);
    float delta = sign(t-s)/size_TF;
    float OCFact = d/(correction_delta*float(steps));

    for(int i = 0; i<steps; i++)
    {
        vec4 c = texture2D(transferFTex, vec2(s,0.0));
        c.a = 1.0 - pow(1.0 - c.a, OCFact);
        c.rgb *= c.a;
        accum += (1- accum.a)*c;
        s += delta;
    }

    accum = clamp(accum, vec4(0.0), vec4(1.0));

    gl_FragColor = accum;
}
