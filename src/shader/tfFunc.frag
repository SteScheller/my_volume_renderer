#version 330 core
layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec2 picking;

in vec2 vTfTexCoord;
in vec2 vQuadCoord;

uniform sampler2D transferTex;  //!< texture that contains the transfer
                                //!< function in the whole definition interval

uniform int width;   //!< width of the frame in pixels
uniform int height;  //!< height of the frame in pixels

#define LINE_THICKNESS 0.02f
#define LINE_COLOR vec4(vec3(0.9f), 1.f)

#define BG_SQUARE_SIZE 8.f
#define BG_COLOR_1 vec4(vec3(0.3f), 1.f)
#define BG_COLOR_2 vec4(vec3(0.1f), 1.f)

void main()
{
    float tfAlpha = texture(transferTex, vTfTexCoord).a;
    int bg_x = 0, bg_y = 0;

    if (abs(vQuadCoord.y - tfAlpha) < (0.5f * LINE_THICKNESS))
    {
        // the fragemnt lies on the transfer function
        frag_color = LINE_COLOR;
    }
    else
    {
        // the fragment belongs to the background
        bg_x = int(floor(gl_FragCoord.x / BG_SQUARE_SIZE));
        bg_y = int(floor(gl_FragCoord.y / BG_SQUARE_SIZE));

        if ((bg_x % 2) > 0)
        {
            if ((bg_y % 2) > 0)
                frag_color = BG_COLOR_1;
            else
                frag_color = BG_COLOR_2;
        }
        else
        {
            if ((bg_y % 2) > 0)
                frag_color = BG_COLOR_2;
            else
                frag_color = BG_COLOR_1;
        }
    }

    picking = vec2(0.f);
}
