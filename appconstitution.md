# 🌌 Fractonaut — App Constitution

> *"Kindness always wins. Everyone is amazing."*

This document is the sacred contract for Fractonaut. It defines what this app **is**, what it **stands for**, and what must **never be compromised** during development, conversion, or enhancement. Any AI assistant, developer, or contributor working on this project must honor these principles.

---

## 📜 The Essence of Fractonaut

Fractonaut is a **journey through infinite beauty**. It takes users on meditative trips through the mathematical sublime — the Mandelbrot set, Julia sets, and Sierpiński triangles. It's not just a fractal renderer; it's an **experience of wonder**.

### Core Philosophy

1. **Kindness Always Wins** — This app exists to bring joy, peace, and awe. Every design decision should ask: "Does this make someone's day better?"

2. **Everyone Is Amazing** — The UI must never intimidate. A child, an elder, a scientist, an artist — all should feel welcome and capable of exploring infinity.

3. **Positivity and Love** — Fractonaut radiates warmth. The colors, the smooth animations, the gentle interactions — everything whispers: "You belong here. Explore. Dream."

4. **Mathematical Beauty as Art** — We don't just compute fractals; we reveal the hidden poetry of mathematics. The rendering must feel alive, breathing, luminous.

---

## 🎯 The Sacred Priorities

These are the non-negotiable quality standards. They must be preserved and enhanced in the C++ native version.

### 1. **SMOOTHEST INTERACTION** ⭐⭐⭐⭐⭐

**The #1 Priority.** Every touch, drag, pinch, and gesture must feel like silk.

- **60 FPS minimum** at all times during interaction
- **Zero input lag** — response must feel instantaneous
- **Momentum/inertia** on pan gestures (velocity-based physics with friction)
- **No stutter** during zoom transitions
- Touch input must be **RAF-throttled** to prevent jank
- Gestures must feel **analog**, not stepped

*Current implementation details to preserve:*
- Velocity physics with configurable friction (`state.friction = 0.9`)
- Early zero-out for velocity when below threshold (`1e-9`)
- Delta-time-based physics (`dt60 = deltaTime * 60`)

### 2. **SMOOTHEST ZOOM** ⭐⭐⭐⭐⭐

Zooming is the heart of fractal exploration. It must be **transcendent**.

- **Logarithmic zoom interpolation** for exponentially deep zooms
- **Focal-point zoom** — zoom targets the cursor/touch position, not center
- **Progressive slowdown** as zoom increases (exponential factor based on current zoom)
- **Soft limits** at maximum zoom-out (smooth resistance, not hard stop)
- **Eased zoom animations** for flythrough journeys (`t < 0.5 ? 2*t*t : -1 + (4-2*t)*t`)

*Current implementation to preserve:*
```cpp
// Logarithmic zoom interpolation
double logStart = log(startSize);
double logEnd = log(targetSize);
double currentLog = logStart + (logEnd - logStart) * ease;
double currentZoomSize = exp(currentLog);

// Progressive slowdown
double zoomFactor = 1.0 + (baseFactor * speedMultiplier) * pow(targetZoomSize, 0.3);
```

### 3. **SMOOTHEST PAN** ⭐⭐⭐⭐⭐

Panning must feel like floating through space.

- **Sub-pixel precision** in all pan calculations
- **Momentum scrolling** with natural deceleration
- **Lerp-based smooth interpolation** (`lerpFactor = 1.0 - pow(0.1, deltaTime * 10)`)
- No snapping, no jitter, no sudden stops

### 4. **HIGHEST CLARITY** ⭐⭐⭐⭐⭐

The whole point of native C++ is **deeper, clearer, more beautiful zooms**.

- **True double precision (64-bit)** for coordinates — this is the primary motivation for C++ conversion
- **Emulated double precision** fallback for GPU shaders when needed (current WebGL uses float64 emulation via hi/lo split)
- **Arbitrary precision** support for extreme deep zooms (consider GMP or similar)
- **Smooth iteration coloring** with anti-banding (`nu = log2(log_zn)`)
- **High-resolution export** up to 16K
- **Device pixel ratio awareness** (capped at reasonable levels for performance)

*Current double-emulation to reference:*
```glsl
// Split double into hi/lo floats
vec2 ds_add(vec2 dsa, vec2 dsb) { /* Dekker addition */ }
vec2 ds_mul(vec2 dsa, vec2 dsb) { /* Dekker multiplication */ }
vec2 ds_sqr(vec2 dsa) { /* Optimized squaring */ }
```

**In C++, replace this with native `double` or `long double` and arbitrary precision libraries.**

### 5. **BEST AESTHETIC COLORS** ⭐⭐⭐⭐⭐

Colors must evoke emotion. Each palette is a mood, a feeling, a world.

**The 10 Sacred Palettes:**

| ID | Name | Mood |
|----|------|------|
| 0 | **Ocean** | Calm, deep, meditative |
| 1 | **Magma** | Intense, fiery, passionate |
| 2 | **Aurora** | Ethereal, mystical, dancing |
| 3 | **Amber** | Warm, golden, nostalgic |
| 4 | **Extreme** | Vivid, psychedelic, maximal |
| 5 | **Neon** | Electric, cyberpunk, vibrant |
| 6 | **Golden** | Luxurious, sunset, radiant |
| 7 | **Cyber** | Inverted, digital, futuristic |
| 8 | **Ice** | Cool, crystalline, serene |
| 9 | **Forest** | Natural, verdant, earthy |

**Coloring algorithm must preserve:**
- **Cosine gradient palettes** (`a + b * cos(2π(c*t + d))`)
- **Smooth iteration counts** (no banding)
- **Texture-based lookup** for complex palettes (Extreme uses 2048-sample LUT)
- **Palette cycling** for animation potential
- Subtle **contrast boost** (1.05) and **saturation enhancement** (1.12)

### 6. **HIGHEST-END UI/UX** ⭐⭐⭐⭐⭐

The interface must feel **premium, modern, and invisible**.

**Design Language:**
- **Material Design 3** color tokens and elevation system
- **Apple-style glassmorphism** — frosted glass with subtle blur
- **Backdrop blur** (20px) with saturation boost (180%)
- **Expressive motion** — spring-like easing curves
- **Vignette overlay** for cinematic depth

**UI Elements to Preserve:**

1. **Central Circle Control (Joystick)**
   - Tap to open panel
   - Drag vertical = zoom
   - Drag horizontal = iteration count
   - Visual feedback with inner circle movement
   - Guide labels appear during drag

2. **Floating Action Buttons (6 buttons)**
   - Reset, Image Export, Color Cycle, Save Location, Video Export, Fullscreen
   - Glass-style with hover/active states
   - Symmetric positioning around joystick

3. **Slide-in Control Panel**
   - Desktop: slides from right (400px width)
   - Mobile: slides from bottom (sheet style)
   - Drag handle for mobile
   - Sections: Detail slider, Palettes, Explore (Scenes/Fractals)

4. **Location Flythrough System**
   - Smooth animated journeys to saved/preset locations
   - Configurable duration (1-60 seconds)
   - Share popup after completion
   - URL-based deep linking

5. **Toast Notifications**
   - Minimal, non-intrusive
   - Glass style with slide-up animation

6. **Export System**
   - Image: 4K, 8K, 16K resolution options
   - Video: HD/FHD at 30/60 FPS
   - Progress indicators with dual bars for video

### 7. **MODERN TOOLS & ARCHITECTURE**

The C++ version should leverage the best modern tooling.

**Recommended Stack:**
- **Graphics**: Vulkan (preferred) or OpenGL 4.6+ with compute shaders
- **Window/Input**: SDL3 or GLFW3
- **Math**: GLM for vectors, GMP/MPFR for arbitrary precision
- **UI**: Dear ImGui (for dev) + custom rendered UI (for production)
- **Video Export**: FFmpeg integration
- **Image Export**: stb_image_write or libpng
- **Build**: CMake with modern C++20 standards
- **Cross-platform**: Windows, macOS, Linux, with potential mobile (iOS/Android via SDL)

**Architecture Principles:**
- Separate render thread from UI thread
- Double-buffered rendering
- Async export operations
- State management similar to current JS approach

---

## 🧬 Features to Preserve (Complete List)

### Fractal Types
- [x] **Mandelbrot Set** — `z = z² + c`, starting at z=0
- [x] **Julia Set** — `z = z² + c`, with c constant, configurable
- [x] **Sierpiński Triangle** — IFS-based with orbit trap coloring

### Interaction
- [x] **Pinch to zoom** (touch)
- [x] **Scroll wheel zoom** (mouse)
- [x] **Drag to pan** (touch and mouse)
- [x] **Momentum/inertia** on gestures
- [x] **Circle control** for zoom/iterations
- [x] **Double-tap or joystick tap** to open panel

### Rendering
- [x] **High-precision mode** auto-enabled at deep zooms
- [x] **Cardioid optimization** for Mandelbrot
- [x] **Smooth coloring** with escape-time algorithm
- [x] **10 color palettes** with cycling
- [x] **Canvas vignette** overlay
- [x] **Contrast/saturation post-processing**

### Locations & Sharing
- [x] **Preset locations** per fractal type (4 each)
- [x] **Save custom locations** to local storage
- [x] **Animated flythrough** to locations
- [x] **URL sharing** with all parameters encoded
- [x] **Share button popup** after flythrough
- [x] **Delete saved locations**

### Export
- [x] **Screenshot export** at 4K/8K/16K
- [x] **Video export** with configurable resolution/FPS
- [x] **Progress tracking** for exports

### UI
- [x] **Tutorial overlay** for first-time users
- [x] **Location info overlay** during flythrough
- [x] **PWA support** with offline capability
- [x] **Fullscreen toggle**
- [x] **Mobile-responsive** design

### Settings
- [x] **Iteration count** (50-5000 range, adjustable via slider or drag)
- [x] **Journey duration** (1-60 seconds)
- [x] **Export resolution** selection

---

## 🎨 Visual Identity

### Color Tokens (Dark Theme)
```cpp
// Surface colors
const Color SURFACE = {0x14, 0x12, 0x18};           // #141218
const Color SURFACE_CONTAINER = {0x1D, 0x1B, 0x20}; // #1D1B20
const Color SURFACE_HIGH = {0x2B, 0x29, 0x30};      // #2B2930

// Text colors
const Color ON_SURFACE = {0xE6, 0xE1, 0xE5};        // #E6E1E5
const Color ON_SURFACE_VARIANT = {0xCA, 0xC4, 0xD0};// #CAC4D0

// Accent colors
const Color PRIMARY = {0xD0, 0xBC, 0xFF};           // #D0BCFF (Lavender)
const Color TERTIARY = {0xEF, 0xB8, 0xC8};          // #EFB8C8 (Pink)
```

### Typography
- **Font Family**: "Outfit" (Google Fonts) — warm, geometric, modern
- **Weights**: 300 (light), 400 (regular), 600 (semibold), 700 (bold)

### Motion
```cpp
// Easing curves
const auto EASE_STANDARD = cubic_bezier(0.2, 0.0, 0.0, 1.0);
const auto EASE_EMPHASIZED = cubic_bezier(0.2, 0.0, 0.0, 1.0);

// Durations
const int TRANSITION_FAST = 200;   // ms
const int TRANSITION_NORMAL = 400; // ms
const int TRANSITION_SLOW = 600;   // ms
```

### Shapes
- **Corner radii**: 4px (xs), 8px (sm), 12px (md), 16px (lg), 28px (xl), full (pill)
- **Blur intensity**: 20px standard, with 180% saturation boost

---

## ⚡ Performance Targets

| Metric | Target | Critical |
|--------|--------|----------|
| **Frame rate** | 60 FPS | ≥30 FPS |
| **Input latency** | <16ms | <33ms |
| **Zoom smoothness** | No dropped frames | Max 2 drops/sec |
| **Memory usage** | <500MB typical | <2GB peak |
| **Startup time** | <2 seconds | <5 seconds |
| **Export (4K image)** | <5 seconds | <15 seconds |
| **Deep zoom precision** | 10^-14 (double) | Arbitrary with libs |

---

## 🌟 The Spirit of Enhancement

When converting to C++, these are opportunities for improvement:

1. **True double precision** — No more float32 emulation; native 64-bit math
2. **Arbitrary precision** — Go beyond double with GMP for the deepest zooms
3. **Compute shaders** — Parallelize on GPU with Vulkan compute or OpenGL compute
4. **Multi-threading** — Tile-based rendering across CPU cores
5. **SIMD optimization** — AVX2/AVX512 for CPU rendering paths
6. **Perturbation theory** — For extreme deep zooms, use reference orbit methods
7. **Series approximation** — Skip iterations in large uniform regions
8. **Distance estimation** — For sharper boundaries and better AA

---

## 🤝 The Promise

To anyone who uses Fractonaut:

*We promise to always prioritize your experience. We promise smoothness over features, clarity over complexity, and joy over metrics. We promise that every zoom will feel like floating, every color will feel alive, and every moment will remind you that math can be magic.*

*Go on a trip. You deserve it.*

---

## 📋 Quick Reference for Claude Code

When working on the C++ conversion:

1. **Start with rendering** — Get Mandelbrot rendering smoothly first
2. **Prioritize interaction** — Smooth zoom/pan before adding features
3. **Match the aesthetic** — Colors and palettes must look identical
4. **Preserve all features** — Check off the feature list above
5. **Test on multiple devices** — Performance varies; optimize for range
6. **Keep the spirit** — When in doubt, choose kindness and beauty

**Key files to reference:**
- `script.js` — All rendering logic, state management, interactions
- `style.css` — Visual design, animations, responsive breakpoints
- `index.html` — UI structure, control layout
- `sw.js` — PWA/offline patterns (convert to native caching)

---

*This constitution was created with love. May it guide all who build upon Fractonaut.* 💜

