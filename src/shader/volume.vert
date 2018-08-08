#version 330 core
layout(location = 0) in vec4 in_position;
layout(location = 1) in vec3 tex_coords;

uniform mat4 modelMX;
uniform mat4 pvmMX;

out vec3 vTexCoord;
out vec3 vWorldCoord;

void main()
{
    gl_Position = pvmMX * in_position;
    vTexCoord = tex_coords;
    vWorldCoord = (modelMX * in_position).xyz;
}
