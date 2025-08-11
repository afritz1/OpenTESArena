#version 450

layout(location = 0) in vec3 vertInPosition;

layout(location = 0) out vec3 fragInColor;

vec3 colors[3] = vec3[]
(
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main()
{
    gl_Position = vec4(vertInPosition, 1.0);
    fragInColor = colors[gl_VertexIndex % 3];
}
