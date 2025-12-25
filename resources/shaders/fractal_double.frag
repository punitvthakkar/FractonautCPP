#version 410 core
#extension GL_ARB_gpu_shader_fp64 : require

uniform vec2 u_resolution;
// Native double precision uniforms
uniform double u_zoomCenter_x;
uniform double u_zoomCenter_y;
uniform double u_zoomSize;
uniform int u_maxIterations;
uniform int u_paletteId;
uniform sampler2D u_paletteTexture;

// Fractal type uniforms
uniform int u_fractalType; // 0: Mandelbrot, 1: Julia, 2: Sierpinski
uniform vec2 u_juliaC;

// Output color
out vec4 outColor;

// Cosine gradient palette function
vec3 palette(float t, vec3 a, vec3 b, vec3 c, vec3 d) {
    return a + b*cos(6.28318*(c*t+d));
}

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * u_resolution.xy) / u_resolution.y;

    float iterations = 0.0;
    bool escaped = false;
    float log_zn = 0.0;

    // Sierpinski Triangle (Type 2) - doesn't need high precision
    if (u_fractalType == 2) {
        vec2 z = vec2(float(u_zoomCenter_x), float(u_zoomCenter_y)) + uv * float(u_zoomSize);

        z.y -= 0.25;

        float scale = 1.0;
        float d = 1000.0;

        for (int i = 0; i < 20; i++) {
            z.x = abs(z.x);
            z.y = abs(z.y);

            if (z.x + z.y > 1.0) {
                float temp = z.x;
                z.x = 1.0 - z.y;
                z.y = 1.0 - temp;
            }

            z *= 2.0;
            z.y -= 1.0;
            scale *= 2.0;

            d = min(d, length(z));
        }

        float t = 0.5 + 0.5 * sin(d * 4.0 + float(u_paletteId));
        vec4 color = texture(u_paletteTexture, vec2(t, 0.0));

        if (length(z) > 2.0) color *= 0.0;

        outColor = vec4(1.0 - color.rgb, 1.0);
        return;
    }

    // Mandelbrot (0) and Julia (1) with NATIVE double precision
    dvec2 c, z;

    // Calculate pixel position in double precision
    dvec2 uv_double = dvec2(double(uv.x), double(uv.y));
    dvec2 center = dvec2(u_zoomCenter_x, u_zoomCenter_y);

    if (u_fractalType == 1) {
        // Julia: c is constant, z is pixel position
        c = dvec2(double(u_juliaC.x), double(u_juliaC.y));
        z = center + uv_double * u_zoomSize;
    } else {
        // Mandelbrot: z starts at 0, c is pixel position
        c = center + uv_double * u_zoomSize;
        z = dvec2(0.0);

        // Cardioid check optimization
        double p = sqrt((c.x - 0.25) * (c.x - 0.25) + c.y * c.y);
        if (c.x < p - 2.0 * p * p + 0.25) {
            outColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }
    }

    // Main fractal iteration loop with native double precision
    for (int i = 0; i < 10000; i++) {
        if (i >= u_maxIterations) break;

        double x = (z.x * z.x - z.y * z.y) + c.x;
        double y = (2.0 * z.x * z.y) + c.y;

        double magnitude_sq = x * x + y * y;
        if (magnitude_sq > 4.0) {
            escaped = true;
            iterations = float(i);
            // GLSL log() only works with float, not double - cast first
            log_zn = float(log(float(magnitude_sq)) / 2.0);
            break;
        }
        z.x = x;
        z.y = y;
    }

    if (escaped) {
        float nu = log2(log_zn);
        float smooth_i = iterations + 1.0 - nu;
        float t = smooth_i / float(u_maxIterations);

        vec3 color = vec3(0.0);

        if (u_paletteId == 0) {
            color = palette(t * 10.0, vec3(0.5), vec3(0.5), vec3(1.0), vec3(0.00, 0.10, 0.20));
        } else if (u_paletteId == 1) {
            color = palette(t * 10.0, vec3(0.5), vec3(0.5), vec3(1.0, 1.0, 0.5), vec3(0.8, 0.9, 0.3));
            color = mix(vec3(0.1, 0.0, 0.0), color, sin(t * 20.0) * 0.5 + 0.5);
        } else if (u_paletteId == 2) {
            color = palette(t * 15.0, vec3(0.5), vec3(0.5), vec3(2.0, 1.0, 0.0), vec3(0.5, 0.20, 0.25));
        } else if (u_paletteId == 3) {
            color = palette(t * 8.0, vec3(0.8, 0.5, 0.4), vec3(0.2, 0.4, 0.2), vec3(2.0, 1.0, 1.0), vec3(0.00, 0.25, 0.25));
        } else if (u_paletteId == 4) {
            float cycle = mod(smooth_i, 512.0) / 512.0;
            color = texture(u_paletteTexture, vec2(cycle, 0.5)).rgb;
        } else if (u_paletteId == 5) {
            color = palette(t * 4.0, vec3(0.5), vec3(0.5), vec3(1.0), vec3(0.3, 0.2, 0.2));
            color = mix(color, vec3(0.0, 1.0, 1.0), sin(t * 10.0) * 0.5 + 0.5);
        } else if (u_paletteId == 6) {
            color = palette(t * 5.0, vec3(0.8, 0.5, 0.4), vec3(0.2, 0.4, 0.2), vec3(2.0, 1.0, 1.0), vec3(0.00, 0.25, 0.25));
            color += vec3(0.2, 0.1, 0.0);
        } else if (u_paletteId == 7) {
            color = palette(t * 6.0, vec3(0.5), vec3(0.5), vec3(2.0, 1.0, 0.0), vec3(0.5, 0.20, 0.25));
            color = vec3(1.0) - color;
        } else if (u_paletteId == 8) {
            color = palette(t * 12.0, vec3(0.5), vec3(0.5), vec3(1.0, 1.0, 1.0), vec3(0.0, 0.33, 0.67));
        } else if (u_paletteId == 9) {
            color = palette(t * 8.0, vec3(0.2, 0.7, 0.4), vec3(0.5, 0.2, 0.3), vec3(1.0), vec3(0.0, 0.1, 0.0));
        }

        outColor = vec4(color, 1.0);
    } else {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
