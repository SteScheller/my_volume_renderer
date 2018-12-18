#version 330 core

uniform mat4 projMX;    //!< projection matrix

uniform float pos;      //!< position of the control point
uniform vec4 color;     //!< RGBA color assigned to that control point

uniform float x_min;    //!< lower limit of the shown interval
uniform float x_max;    //!< upper limit of the shown interval

void main()
{
    float pos_x = 0.f, pos_y = 0.f;

    pos_x = (pos - x_min) / (x_max - x_min);

    // y is flipped for use of texture in ImGui Image
    pos_y = 1.f - color.a;

    gl_Position = projMX * vec4(pos_x - 0.5f, pos_y - 0.5f, 0.f, 1.f);
}
