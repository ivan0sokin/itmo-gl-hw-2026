#version 330 core

uniform sampler2D tex_2d;
uniform vec4 color;

in vec3 vert_col;
in vec2 vert_tex;

out vec4 out_col;

void main() {
    vec4 texel = texture(tex_2d, vec2(vert_tex.x, 1.0 - vert_tex.y));
    
    float greyscale_factor = dot(texel.rgb, vec3(0.21, 0.71, 0.07));
    vec3 semi_greyscale = mix(vec3(greyscale_factor), vert_col.rgb, 0.7f);

    vec3 animated_color = mix(color.rgb, semi_greyscale, 0.7f);

    out_col = vec4(animated_color, 1.0);
}
