#version 330 core
layout(location = 0) out vec4 frag_color;

in vec3 vTexCoord;

void main()
{
    frag_color = vec4(vTexCoord, 1.f);
}
