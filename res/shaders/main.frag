uniform vec2 resolution;

uniform float chess_squaresize;
uniform float rainbow_p;
uniform float twist_factor; // try 4.0

void twist(out vec2 uv, in vec2 uv1);
void mirror(out vec2 uv, in vec2 uv1);
void rainbow(out vec3 col, in vec2 uv);
void chess(out vec3 col, in vec2 uv, in vec3 color1, in vec3 color2);
void invert(out vec3 col, in vec3 col1);

void main()
{
    // normalized coordinates
    vec2 uv = gl_FragCoord.xy/resolution.xy;
    vec2 uv3;
    twist(uv3, uv);
    vec2 uv2;
    mirror(uv2,uv3);
    vec3 col;
    rainbow(col, uv2);
    vec3 col2;
    invert(col2, col);
    vec3 col3;
    chess(col3,uv3,col,col2);

    gl_FragColor = vec4(col3,1.0);
}