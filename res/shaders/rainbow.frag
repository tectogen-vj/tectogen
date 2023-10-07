uniform float rainbow_p;

void rainbow(out vec3 col, in vec2 uv)
{
    float fac=0.001*rainbow_p;
    col = 0.5 + 0.5*cos(fac+uv.xyx*8.0+vec3(0,2,4));
}
