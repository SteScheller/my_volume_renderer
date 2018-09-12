#version 330 core
layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 tex_coords;

uniform mat4 projMX;    //!< projection matrix

out vec2 vTexCoord;     //!< coordinates for mapping the image to the quad

void main()
{
    gl_Position = projMX * vec4(in_position, 0.f, 1.f);
    vTexCoord = tex_coords;
}
