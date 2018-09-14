#version 330 core
layout(location = 0) out vec4 frag_color;

in vec2 vTexCoord;

uniform sampler2D renderTex;    //!< texture that contains the 2D result of
                                //!< the volume rendering
uniform usampler2D rngTex;      //!< texture that contains the fragment state
                                //!< of the random number generator texture
uniform int texSelect;          //!< selection which texture shall be shown

void main()
{
    uvec4 state = uvec4(0U);

    switch(texSelect)
    {
        case 1: // random number generator texture
            state = texture(rngTex, vTexCoord);
            frag_color.r = float(state.x % 256U) / 255.f;
            frag_color.g = float(state.y % 256U) / 255.f;
            frag_color.b = float(state.z % 256U) / 255.f;
            frag_color.a = float(state.w % 256U) / 255.f;
            break;

        case 0:
        default:
            frag_color = texture(renderTex, vTexCoord);
            break;
    }
}

