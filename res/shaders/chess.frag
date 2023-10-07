uniform float chess_squaresize;

void chess(out vec3 col, in vec2 uv, in vec3 color1, in vec3 color2) {
    vec2 arg = vec2(chess_squaresize/4., chess_squaresize/4.);
    vec2 m = mod(uv.xy, 2.*arg);
    if (m.x < arg.x && m.y < arg.y || m.x >= arg.x && m.y >= arg.y)
        col=color1;
    else
        col=color2;
}