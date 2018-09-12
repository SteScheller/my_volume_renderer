#version 330 core
layout(location = 0) out vec4 frag_color;

in vec2 vTexCoord;

uniform sampler2D tex;  //!< texture that contains the 2D result of the volume
                        //!< rendering

void main()
{
    frag_color = texture(tex, vTexCoord);
}

