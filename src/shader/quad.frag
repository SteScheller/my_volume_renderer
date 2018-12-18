#version 330 core
layout(location = 0) out vec4 fragColor;

in vec2 vTexCoord;

uniform sampler2D renderTex;    //!< texture that contains the 2D result of
                                //!< the volume rendering
uniform usampler2D rngTex;      //!< texture that contains the fragment state
                                //!< of the random number generator texture
uniform sampler3D volumeTex;    //!< 3D texture that contains the volume data
uniform int texSelect;          //!< selection which texture shall be shown

uniform float volumeZ;          //!< z coordinate for volume sampling

void main()
{
    uvec4 state = uvec4(0U);
    float value = 0.f;

    switch(texSelect)
    {
        case 1: // random number generator texture
            state = texture(rngTex, vTexCoord);
            fragColor.r = float(state.x % 256U) / 255.f;
            fragColor.g = float(state.y % 256U) / 255.f;
            fragColor.b = float(state.z % 256U) / 255.f;
            fragColor.a = float(state.w % 256U) / 255.f;
            break;

        case 2: // volume texture
            value = texture(volumeTex, vec3(vTexCoord, volumeZ)).r;
            fragColor = vec4(vec3(value), 1.f);
            break;

        case 0:
        default:
            fragColor = texture(renderTex, vTexCoord);
            break;
    }
}

