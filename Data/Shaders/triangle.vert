#version 460

vec2 vertices[] = vec2[3](
    vec2(-.5, 0.0),
    vec2(0.5, 0.5),
    vec2(0.5, -.5)
);

layout(push_constant) uniform PushConstant
{
    mat4 projView;
};

void main()
{
    gl_Position = projView * vec4(-5, vertices[gl_VertexIndex].xy, 1);
}