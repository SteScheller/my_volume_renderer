#version 330 core
layout(location = 0) in vec2 in_position;
layout(location = 1) in float x_val;

uniform mat4 projMX;

out vec2 vTexCoord;

void main()
{
    gl_Position = projMX * in_position;
    vTexCoord = vec2(x_val, 0.5f);
}
