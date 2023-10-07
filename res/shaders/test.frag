void test(out vec3 col, in vec2 uv)
{
    col = vec3(uv.x, uv.y, mod(uv.x*10.0,1.0));
}
