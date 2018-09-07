#version 330 core
layout(location = 0) out vec4 frag_color;
layout(location = 1) out float picking;

uniform float pos;  //!< position of the control point
uniform vec4 color; //!< RGBA color assigned to that control point

void main()
{
    frag_color = vec4(color.rgb, 1.f);
    picking = pos;
}
