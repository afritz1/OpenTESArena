#version 450

layout(location = 0) in vec2 vertInPosition;
layout(location = 1) in vec2 vertInTexCoord;

layout(location = 0) out vec2 fragInTexCoord;

void main()
{
    gl_Position = vec4(vertInPosition, 1.0, 1.0);
    fragInTexCoord = vertInTexCoord;
}
