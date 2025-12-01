#version 410 core

uniform vec2 u_resolution;
// Double precision emulation: .x = high, .y = low
// Double precision emulation: .x = high, .y = low
uniform float u_zoomCenter_x_hi;
uniform float u_zoomCenter_x_lo;
uniform float u_zoomCenter_y_hi;
uniform float u_zoomCenter_y_lo;
uniform float u_zoomSize_hi;
uniform float u_zoomSize_lo;
uniform int u_maxIterations;
uniform int u_paletteId;
uniform bool u_highPrecision;
uniform sampler2D u_paletteTexture;

// Fractal type uniforms
uniform int u_fractalType; // 0: Mandelbrot, 1: Julia, 2: Sierpinski
uniform vec2 u_juliaC;

// Output color
out vec4 outColor;

// Constants
const float split = 8193.0;

// Emulated double math functions for deep zoom
vec2 ds_add(vec2 dsa, vec2 dsb) {
    vec2 dsc;
    float t1, t2, e;
    t1 = dsa.x + dsb.x;
    e = t1 - dsa.x;
    t2 = ((dsb.x - e) + (dsa.x - (t1 - e))) + dsa.y + dsb.y;
    dsc.x = t1 + t2;
    dsc.y = t2 - (dsc.x - t1);
    return dsc;
}

vec2 ds_sub(vec2 dsa, vec2 dsb){
    vec2 dsc;
    float t1, t2, e;
    t1 = dsa.x - dsb.x;
    e = t1 - dsa.x;
    t2 = ((-dsb.x - e) + (dsa.x - (t1 - e))) + dsa.y - dsb.y;
    dsc.x = t1 + t2;
    dsc.y = t2 - (dsc.x - t1);
    return dsc;
}

vec2 ds_mul(vec2 dsa, vec2 dsb) {
    vec2 dsc;
    float c11, c21, c2, e, t1, t2;
    float a1, a2, b1, b2, cona, conb;
    
    cona = dsa.x * split;
    a1 = cona - (cona - dsa.x);
    a2 = dsa.x - a1;
    
    conb = dsb.x * split;
    b1 = conb - (conb - dsb.x);
    b2 = dsb.x - b1;
    
    c11 = dsa.x * dsb.x;
    c21 = a1 * b1 - c11;
    c21 += a1 * b2;
    c21 += a2 * b1;
    c21 += a2 * b2;
    
    c2 = dsa.x * dsb.y + dsa.y * dsb.x;
    
    t1 = c11 + c2;
    e = t1 - c11;
    t2 = dsa.y * dsb.y + ((c2 - e) + (c11 - (t1 - e))) + c21;
    
    dsc.x = t1 + t2;
    dsc.y = t2 - (dsc.x - t1);
    return dsc;
}

vec2 ds_sqr(vec2 dsa) {
    vec2 dsc;
    float c11, c21, c2, e, t1, t2;
    float a1, a2, cona;
    
    cona = dsa.x * split;
    a1 = cona - (cona - dsa.x);
    a2 = dsa.x - a1;
    
    c11 = dsa.x * dsa.x;
    c21 = a1 * a1 - c11;
    c21 += 2.0 * a1 * a2;
    c21 += a2 * a2;
    
    c2 = 2.0 * dsa.x * dsa.y;
    
    t1 = c11 + c2;
    e = t1 - c11;
    t2 = dsa.y * dsa.y + ((c2 - e) + (c11 - (t1 - e))) + c21;
    
    dsc.x = t1 + t2;
    dsc.y = t2 - (dsc.x - t1);
    return dsc;
}

// Cosine gradient palette function
vec3 palette(float t, vec3 a, vec3 b, vec3 c, vec3 d) {
    return a + b*cos(6.28318*(c*t+d));
}

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * u_resolution.xy) / u_resolution.y;
    
    float iterations = 0.0;
    bool escaped = false;
    float log_zn = 0.0;
    
    // Sierpinski Triangle (Type 2)
    if (u_fractalType == 2) {
        vec2 z = vec2(u_zoomCenter_x_hi, u_zoomCenter_y_hi) + uv * u_zoomSize_hi;
        
        // Center correction
        z.y -= 0.25; 
        
        float scale = 1.0;
        float d = 1000.0;
        
        for (int i = 0; i < 20; i++) { // Fixed iterations for IFS
            z.x = abs(z.x);
            z.y = abs(z.y);
            
            if (z.x + z.y > 1.0) {
                float temp = z.x;
                z.x = 1.0 - z.y;
                z.y = 1.0 - temp;
            }
            
            z *= 2.0;
            z.y -= 1.0; // Standard Gasket shift
            scale *= 2.0;
            
            // Trap for coloring
            d = min(d, length(z));
        }
        
        // Coloring based on trap distance
        float t = 0.5 + 0.5 * sin(d * 4.0 + float(u_paletteId));
        vec4 color = texture(u_paletteTexture, vec2(t, 0.0));
        
        // Make background black-ish
        if (length(z) > 2.0) color *= 0.0;
        
        // Invert colors completely (Sierpinski only)
        outColor = vec4(1.0 - color.rgb, 1.0);
        return;
    }

    // Mandelbrot (0) and Julia (1) Logic
    if (u_highPrecision) {
        vec2 uv_x_ds = vec2(uv.x, 0.0);
        vec2 uv_y_ds = vec2(uv.y, 0.0);

        vec2 c_x, c_y;
        vec2 z_x, z_y;
        
        // Reconstruct DS numbers from uniforms
        vec2 zoomCenter_x = vec2(u_zoomCenter_x_hi, u_zoomCenter_x_lo);
        vec2 zoomCenter_y = vec2(u_zoomCenter_y_hi, u_zoomCenter_y_lo);
        vec2 zoomSize = vec2(u_zoomSize_hi, u_zoomSize_lo);

        if (u_fractalType == 1) {
            // Julia: c is constant, z is pixel
            c_x = vec2(u_juliaC.x, 0.0);
            c_y = vec2(u_juliaC.y, 0.0);
            z_x = ds_add(zoomCenter_x, ds_mul(uv_x_ds, zoomSize));
            z_y = ds_add(zoomCenter_y, ds_mul(uv_y_ds, zoomSize));
        } else {
            // Mandelbrot: z starts at 0, c is pixel
            c_x = ds_add(zoomCenter_x, ds_mul(uv_x_ds, zoomSize));
            c_y = ds_add(zoomCenter_y, ds_mul(uv_y_ds, zoomSize));
            z_x = vec2(0.0);
            z_y = vec2(0.0);
        }
        
        for (int i = 0; i < 10000; i++) {
            if (i >= u_maxIterations) break;
            
            vec2 z_x2 = ds_sqr(z_x);
            vec2 z_y2 = ds_sqr(z_y);
            
            if (z_x2.x + z_y2.x > 4.0) {
                escaped = true;
                iterations = float(i);
                log_zn = log2(z_x2.x + z_y2.x) / 2.0;
                break;
            }

            vec2 z_xy = ds_mul(z_x, z_y);
            vec2 two_z_xy = ds_add(z_xy, z_xy);
            vec2 new_y = ds_add(two_z_xy, c_y);

            vec2 diff_sq = ds_sub(z_x2, z_y2);
            vec2 new_x = ds_add(diff_sq, c_x);

            z_x = new_x;
            z_y = new_y;
        }
    } else {
        vec2 c, z;
        
        if (u_fractalType == 1) {
            // Julia
            c = u_juliaC;
            z = vec2(u_zoomCenter_x_hi, u_zoomCenter_y_hi) + uv * u_zoomSize_hi;
        } else {
            // Mandelbrot
            c = vec2(u_zoomCenter_x_hi, u_zoomCenter_y_hi) + uv * u_zoomSize_hi;
            z = vec2(0.0);
            
            // Cardioid check optimization (Mandelbrot only)
            if (u_fractalType == 0) {
                float p = sqrt((c.x - 0.25) * (c.x - 0.25) + c.y * c.y);
                if (c.x < p - 2.0 * p * p + 0.25) {
                    iterations = float(u_maxIterations);
                    // Skip loop
                    outColor = vec4(0.0, 0.0, 0.0, 1.0);
                    return;
                } 
            }
        }

        for (int i = 0; i < 10000; i++) {
            if (i >= u_maxIterations) break;
            
            float x = (z.x * z.x - z.y * z.y) + c.x;
            float y = (2.0 * z.x * z.y) + c.y;
            
            if (x * x + y * y > 4.0) {
                escaped = true;
                iterations = float(i);
                log_zn = log2(x * x + y * y) / 2.0;
                break;
            }
            z.x = x;
            z.y = y;
        }
    }

    if (escaped) {
        float nu = log2(log_zn);
        float smooth_i = iterations + 1.0 - nu;
        float t = smooth_i / float(u_maxIterations);
        
        vec3 color = vec3(0.0);

        if (u_paletteId == 0) {
            // Ocean
            color = palette(t * 10.0, vec3(0.5), vec3(0.5), vec3(1.0), vec3(0.00, 0.10, 0.20));
        } else if (u_paletteId == 1) {
            // Magma
            color = palette(t * 10.0, vec3(0.5), vec3(0.5), vec3(1.0, 1.0, 0.5), vec3(0.8, 0.9, 0.3));
            color = mix(vec3(0.1, 0.0, 0.0), color, sin(t * 20.0) * 0.5 + 0.5);
        } else if (u_paletteId == 2) {
            // Aurora
            color = palette(t * 15.0, vec3(0.5), vec3(0.5), vec3(2.0, 1.0, 0.0), vec3(0.5, 0.20, 0.25));
        } else if (u_paletteId == 3) {
            // Amber
            color = palette(t * 8.0, vec3(0.8, 0.5, 0.4), vec3(0.2, 0.4, 0.2), vec3(2.0, 1.0, 1.0), vec3(0.00, 0.25, 0.25));
        } else if (u_paletteId == 4) {
            // Extreme (Texture)
            float cycle = mod(smooth_i, 512.0) / 512.0;
            color = texture(u_paletteTexture, vec2(cycle, 0.5)).rgb;
        } else if (u_paletteId == 5) {
            // Neon
            color = palette(t * 4.0, vec3(0.5), vec3(0.5), vec3(1.0), vec3(0.3, 0.2, 0.2));
            color = mix(color, vec3(0.0, 1.0, 1.0), sin(t * 10.0) * 0.5 + 0.5);
        } else if (u_paletteId == 6) {
            // Golden
            color = palette(t * 5.0, vec3(0.8, 0.5, 0.4), vec3(0.2, 0.4, 0.2), vec3(2.0, 1.0, 1.0), vec3(0.00, 0.25, 0.25));
            color += vec3(0.2, 0.1, 0.0); 
        } else if (u_paletteId == 7) {
            // Cyber
            color = palette(t * 6.0, vec3(0.5), vec3(0.5), vec3(2.0, 1.0, 0.0), vec3(0.5, 0.20, 0.25));
            color = vec3(1.0) - color; // Invert
        } else if (u_paletteId == 8) {
            // Ice
            color = palette(t * 12.0, vec3(0.5), vec3(0.5), vec3(1.0, 1.0, 1.0), vec3(0.0, 0.33, 0.67));
        } else if (u_paletteId == 9) {
            // Forest
            color = palette(t * 8.0, vec3(0.2, 0.7, 0.4), vec3(0.5, 0.2, 0.3), vec3(1.0), vec3(0.0, 0.1, 0.0));
        }
        
        outColor = vec4(color, 1.0);
    } else {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
