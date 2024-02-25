uniform float colorful_p;

void colorful(out vec3 col, in vec2 uv)
{
    float colorful_pe=colorful_p*0.0001;
    vec2 center=vec2(0.5,0.5);
    col=vec3(
                mod(sin(((0.5-uv.x+sin(colorful_pe*0.9+uv.y)*0.2)*(sin(colorful_pe*0.124)*0.5-uv.y*(sin(colorful_pe*0.23)+1.0)))*100.0)*0.5+0.51,sin(colorful_pe*0.26)*2.0+2.5),
                sin(uv.y+colorful_pe*0.3)*0.3+0.3,
                mod(sin(sqrt(pow(center.x-uv.x+sin(colorful_pe*0.5+uv.x)*0.2,2.0)+pow(center.y-uv.y,2.0))*10.0*(sin(colorful_pe*0.2)+1.0)+colorful_pe)*0.5+0.51,sin(colorful_pe*0.26)*2.0+2.5));
}