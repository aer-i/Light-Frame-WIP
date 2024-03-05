#version 460

vec2 vertices[] = vec2[3](
    vec2(0.0, -.5),
    vec2(0.5, 0.5),
    vec2(-.5, 0.5)
);

void main()
{
    gl_Position = vec4(vertices[gl_VertexIndex], 0, 1);
}