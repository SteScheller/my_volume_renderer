#version 330 core
layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 tex_coords;

uniform mat4 projMX;    //!< projection matrix

uniform float x_min;    //!< lower limit of the shown interval
uniform float x_max;    //!< upper limit of the shown interval

uniform float tf_interval_lower;    //!< lower limit of the transfer function
                                    //!< definition interval
uniform float tf_interval_upper;    //!< upper limit of the transfer function
                                    //!< definition interval

out vec2 vTexCoord;     //!< texture coordinates for sampling the transfer
                        //!< function. Normalized such that
                        //!< whole definition interval

void main()
{
    gl_Position = projMX * in_position;

    //TODO: check this calculation
    vTexCoord.x = tex_coords.x *
        ((x_max - x_min) / (tf_interval_upper - tf_interval_lower)) +
        (x_min / (tf_interval_upper;
    vTexCoord.y = 0.5f;
}
