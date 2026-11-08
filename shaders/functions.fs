#version 330 core

uniform vec2 resolution;
uniform vec3 clear_color;
uniform vec2 view_center;
uniform float view_half_height;
uniform float grid_step;
uniform float pixel_scale;
uniform bool show_grid;
uniform bool show_sine;
uniform bool show_hyperbola;
uniform float period;
uniform vec2 hyperbola_center;
uniform vec2 hyperbola_axes;

out vec4 out_col;

bool curve(float value) {
    float gradient = length(vec2(dFdx(value), dFdy(value)));
    float distance = abs(value) / max(gradient, 0.000001);
    return distance <= pixel_scale;
}

bool grid(vec2 point, float spacing, float units_per_pixel, float radius) {
    vec2 distance = abs(fract(point / spacing + 0.5) - 0.5) * spacing;
    return min(distance.x, distance.y) / units_per_pixel <= max(radius, 0.5);
}

void main() {
    float units_per_pixel = 2.0 * view_half_height / resolution.y;
    vec2 point = view_center + (gl_FragCoord.xy - resolution * 0.5) * units_per_pixel;
    vec3 color = clear_color;

    if (show_grid) {
        bool minor = grid(point, grid_step / 5.0, units_per_pixel, 0.3 * pixel_scale);
        bool major = grid(point, grid_step, units_per_pixel, 0.5 * pixel_scale);
        bool axes = min(abs(point.x), abs(point.y)) / units_per_pixel <= 0.7 * pixel_scale;
        if (minor) {
            color = vec3(0.09);
        }
        if (major) {
            color = vec3(0.2);
        }
        if (axes) {
            color = vec3(0.4);
        }
    }

    if (show_sine) {
        float value = point.y - sin(6.28318530718 * point.x / period);
        bool on_curve = curve(value);
        if (on_curve && abs(point.y) <= 1.0 + pixel_scale * units_per_pixel) {
            color = vec3(1.0, 0.65, 0.2);
        }
    }

    if (show_hyperbola) {
        vec2 position = (point - hyperbola_center) / hyperbola_axes;
        float value = position.x * position.x - position.y * position.y - 1.0;
        if (curve(value)) {
            color = vec3(0.2, 0.75, 1.0);
        }
    }

    out_col = vec4(color, 1.0);
}
