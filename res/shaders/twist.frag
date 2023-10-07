uniform float twist_factor; // try 4.0

void twist(out vec2 uv, in vec2 uv1)
{
    vec2 center = vec2(0.5, 0.5);
    vec2 delta = uv1 - center;
    float dist = length(delta);
    
    float angle = dist * twist_factor;
    
    float c = cos(angle);
    float s = sin(angle);
    mat2 rotationMatrix = mat2(c, -s, s, c);
    uv = center + rotationMatrix * delta;
}