#version 330 core
layout(location = 0) out vec4 frag_color;

in vec2 vTexCoord;

uniform sampler2D transferTex;  //!< texture that contains the transfer
                                //!< function in the whole definition interval

void main()
{
    frag_color = texture(transferTex, vTexCoord);
}

