void mirror(out vec2 uv, in vec2 uv1)
{
    if(uv1.x>0.5) {
        uv.x=1.0-uv1.x;
    } else {
        uv.x=uv1.x;
    }
    if(uv1.y>0.5) {
        uv.y=1.0-uv1.y;
    } else {
        uv.y=uv1.y;
    }
}