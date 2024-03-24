#version 460

layout (location = 0) out vec2 outUv;

vec2 gridPlane[] = vec2[](
    vec2(-1, -1),
    vec2( 3, -1),
    vec2(-1,  3)
);

void main() 
{
	gl_Position = vec4(gridPlane[gl_VertexIndex], 0, 1);
	outUv = vec2(0.5, -0.5) * gl_Position.xy + vec2(0.5);
}