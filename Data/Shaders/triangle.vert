#version 460

vec2 vertices[] = vec2[3](
    vec2(-.5, 0.0),
    vec2(0.5, 0.5),
    vec2(0.5, -.5)
);

layout(binding = 0) restrict readonly uniform CameraUniformBuffer
{
    mat4 projection;
    mat4 view;
    mat4 projView;
};

void main()
{
    gl_Position = projView * vec4(-5, vertices[gl_VertexIndex].xy, 1);
}