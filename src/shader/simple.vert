#version 330 core
layout(location = 0) in vec4 in_position;
layout(location = 1) in vec3 tex_coords;

uniform mat4 viewMX;
uniform mat4 projMX;
uniform mat4 pvMX;

out vec3 vTexCoord;

void main()
{
    gl_Position = pvMX * in_position;
    vTexCoord = tex_coords;
}
