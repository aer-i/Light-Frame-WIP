#version 460

vec2 gridPlane[4] = vec2[](
    vec2(-1, -1),
    vec2( 1, -1),
    vec2( 1,  1),
    vec2(-1,  1)
);

struct Camera { mat4 proj; mat4 view; mat4 projView; };

layout(binding = 0) restrict readonly uniform UniformBuffer { Camera camera; };

layout(location = 0) out vec3 nearPoint;
layout(location = 1) out vec3 farPoint;
layout(location = 2) out mat4 fragView;
layout(location = 6) out mat4 fragProj;

vec3 UnprojectPoint(float x, float y, float z)
{
    mat4 viewInv = inverse(camera.view);
    mat4 projInv = inverse(camera.proj);
    vec4 unprojectedPoint = viewInv * projInv * vec4(x, y, z, 1);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

void main()
{
    vec2 p = gridPlane[gl_VertexIndex];

    nearPoint = UnprojectPoint(p.x, p.y, 0.0).xyz;
    farPoint = UnprojectPoint(p.x, p.y, 1.0).xyz;
    fragView = camera.view;
    fragProj = camera.proj;

    gl_Position = vec4(p, 0, 1);
}