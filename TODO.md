# Quantum Wave Sphere

## Definition of Done

The finished project must show:

- a procedurally generated icosphere;
- deformation of its surface in the vertex shader;
- interference of four spherical waves;
- unlit coloring based on the wave-field amplitude;
- a simple Dear ImGui panel;
- automatic sphere rotation;
- a wireframe toggle;
- builds for Windows and Linux.

The first version does not include:

- lighting or PBR;
- textures;
- bloom;
- an orbit camera;
- a compute shader;
- shader hot reload;
- a physically accurate quantum model;
- a material system;
- a custom engine framework.

---

# Sequential TODO

## 1. Prepare the bootstrap for a static sphere

- [x] Keep the existing GLFW window, OpenGL context, and render loop.
- [x] Keep the working `ShaderProgram`.
- [x] Enable the depth test:

```cpp
glEnable(GL_DEPTH_TEST);
```

- [x] Keep back-face culling disabled for now so winding errors do not hide the geometry.
- [x] Set the viewport at startup and whenever the window is resized.
- [ ] Clear the depth buffer together with the screen:

```cpp
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

**Check:** the window opens, shader errors are printed to the console, and the bootstrap triangle is still visible.

---

## 2. Create a CPU representation of the mesh

Minimal structure:

```cpp
struct MeshData {
    std::vector<glm::vec3> positions;
    std::vector<std::uint32_t> indices;
};
```

Function:

```cpp
[[nodiscard]]
MeshData makeIcosphere(std::uint32_t subdivisions);
```

Do not add `Mesh`, `Geometry`, `Renderer`, `Scene`, `Entity`, or any of the other words people use to disguise premature architecture yet.

---

## 3. Create the base icosahedron

The golden ratio:

$$
\varphi = \frac{1 + \sqrt{5}}{2}
$$

Twelve vertices:

```text
(-1, +φ,  0)   (+1, +φ,  0)
(-1, -φ,  0)   (+1, -φ,  0)

( 0, -1, +φ)   ( 0, +1, +φ)
( 0, -1, -φ)   ( 0, +1, -φ)

(+φ,  0, -1)   (+φ,  0, +1)
(-φ,  0, -1)   (-φ,  0, +1)
```

Normalize every vertex:

```cpp
position = glm::normalize(position);
```

Indices for the twenty faces:

```cpp
{
     0, 11,  5,
     0,  5,  1,
     0,  1,  7,
     0,  7, 10,
     0, 10, 11,

     1,  5,  9,
     5, 11,  4,
    11, 10,  2,
    10,  7,  6,
     7,  1,  8,

     3,  9,  4,
     3,  4,  2,
     3,  2,  6,
     3,  6,  8,
     3,  8,  9,

     4,  9,  5,
     2,  4, 11,
     6,  2, 10,
     8,  6,  7,
     9,  8,  1
};
```

**Check:**

```cpp
positions.size() == 12
indices.size() == 60
```

---

## 4. Implement subdivision

For every triangle:

```text
       a
      / \
    ab---ca
    / \ / \
   b---bc---c
```

The original triangle `(a, b, c)` becomes four triangles:

```text
(a,  ab, ca)
(b,  bc, ab)
(c,  ca, bc)
(ab, bc, ca)
```

The midpoint between vertices $\mathbf a$ and $\mathbf b$:

$$
\mathbf m =
\operatorname{normalize}
\left(
    \frac{\mathbf a + \mathbf b}{2}
\right)
$$

The same edge is shared by two triangles, so the midpoint must be cached.

Edge key:

```cpp
const auto low = std::min(a, b);
const auto high = std::max(a, b);

const std::uint64_t key =
    (static_cast<std::uint64_t>(low) << 32) | high;
```

Cache:

```cpp
std::unordered_map<std::uint64_t, std::uint32_t> midpointCache;
```

Recommended level:

```cpp
makeIcosphere(4);
```

Expected vertex count after $n$ subdivisions:

$$
V_n = 10 \cdot 4^n + 2
$$

Triangle count:

$$
F_n = 20 \cdot 4^n
$$

| Subdivisions | Vertices | Triangles |
|-------------:|---------:|----------:|
| 0 | 12 | 20 |
| 1 | 42 | 80 |
| 2 | 162 | 320 |
| 3 | 642 | 1280 |
| 4 | 2562 | 5120 |

**Check:**

- every vertex has a length of approximately `1.0`;
- every index is less than `positions.size()`;
- subdivision level `4` produces `2562` vertices and `5120` triangles.

---

## 5. Upload the sphere to the GPU

Create:

- one VAO;
- one VBO;
- one EBO.

Modern DSA version:

```cpp
glCreateVertexArrays(1, &vao);
glCreateBuffers(1, &vbo);
glCreateBuffers(1, &ebo);

glNamedBufferStorage(
    vbo,
    static_cast<GLsizeiptr>(
        positions.size() * sizeof(glm::vec3)
    ),
    positions.data(),
    0
);

glNamedBufferStorage(
    ebo,
    static_cast<GLsizeiptr>(
        indices.size() * sizeof(std::uint32_t)
    ),
    indices.data(),
    0
);

glVertexArrayVertexBuffer(
    vao,
    0,
    vbo,
    0,
    sizeof(glm::vec3)
);

glEnableVertexArrayAttrib(vao, 0);

glVertexArrayAttribFormat(
    vao,
    0,
    3,
    GL_FLOAT,
    GL_FALSE,
    0
);

glVertexArrayAttribBinding(vao, 0, 0);
glVertexArrayElementBuffer(vao, ebo);
```

Drawing:

```cpp
glBindVertexArray(vao);

glDrawElements(
    GL_TRIANGLES,
    static_cast<GLsizei>(indices.size()),
    GL_UNSIGNED_INT,
    nullptr
);
```

**Check:** a static round sphere is displayed in a single color.

---

## 6. Add camera matrices

Use a simple fixed camera:

```cpp
const glm::mat4 model =
    glm::rotate(
        glm::mat4{1.0f},
        rotation,
        glm::vec3{0.0f, 1.0f, 0.0f}
    );

const glm::mat4 view =
    glm::lookAt(
        glm::vec3{0.0f, 0.0f, 3.0f},
        glm::vec3{0.0f},
        glm::vec3{0.0f, 1.0f, 0.0f}
    );

const glm::mat4 projection =
    glm::perspective(
        glm::radians(45.0f),
        framebufferWidth /
            static_cast<float>(framebufferHeight),
        0.1f,
        100.0f
    );
```

Vertex shader:

```glsl
gl_Position =
    uProjection *
    uView *
    uModel *
    vec4(position, 1.0);
```

**Check:** the sphere rotates slowly and does not stretch when the window is resized.

---

## 7. Implement one wave

The original sphere vertex is a unit direction:

```glsl
vec3 direction = normalize(aPosition);
```

The wave source is also represented by a unit direction:

```glsl
uniform vec3 uSourceDirection;
```

Let:

- $\mathbf n$ be the direction of the current vertex;
- $\mathbf s$ be the wave-source direction;
- $\theta$ be the angular distance between them.

For unit vectors:

$$
\mathbf n \cdot \mathbf s = \cos \theta
$$

Therefore:

$$
\theta =
\arccos
\left(
    \operatorname{clamp}
    \left(
        \mathbf n \cdot \mathbf s,
        -1,
        1
    \right)
\right)
$$

GLSL:

```glsl
float angularDistance(vec3 a, vec3 b)
{
    float cosine =
        clamp(dot(a, b), -1.0, 1.0);

    return acos(cosine);
}
```

`clamp` is required. Because of floating-point error, the dot product can occasionally be slightly greater than `1.0`; `acos` then produces `NaN`, and the sphere departs for the mathematical afterlife.

---

## 8. Evaluate the wave value

Convenient parameters:

```glsl
uniform float uAmplitude;
uniform float uCycles;
uniform float uSpeed;
uniform float uDecay;
uniform float uTime;
```

Let `uCycles` mean the number of complete oscillations from the source to the antipodal point of the sphere.

The angular distance is in the range:

$$
0 \leq \theta \leq \pi
$$

To fit $C$ complete cycles between $\theta = 0$ and $\theta = \pi$, the spatial phase must change by $2\pi C$:

$$
2 C \theta
$$

At $\theta = \pi$, this gives:

$$
2 C \pi
$$

The complete wave equation:

$$
h =
A e^{-D\theta}
\sin
\left(
    2C\theta - \omega t + \phi
\right)
$$

Where:

- $A$ is the amplitude;
- $D$ is the decay coefficient;
- $C$ is the number of cycles to the antipodal point of the sphere;
- $\omega$ is the phase-change speed;
- $t$ is time;
- $\phi$ is the initial phase.

GLSL:

```glsl
float wave(
    vec3 direction,
    vec3 sourceDirection,
    float phaseOffset
) {
    float theta =
        angularDistance(
            direction,
            sourceDirection
        );

    float envelope =
        exp(-uDecay * theta);

    float phase =
        2.0 * uCycles * theta
        - uSpeed * uTime
        + phaseOffset;

    return
        uAmplitude *
        envelope *
        sin(phase);
}
```

Deformation:

```glsl
float height =
    wave(
        direction,
        uSourceDirection,
        0.0
    );

vec3 displacedPosition =
    direction * (1.0 + height);
```

In mathematical form:

$$
\mathbf p =
\mathbf n (R + h)
$$

For a unit base sphere, $R = 1$:

$$
\mathbf p =
\mathbf n (1 + h)
$$

**Check:** concentric rings spread across the sphere from a single point.

---

## 9. Add four sources

In the minimal version, every source uses the same:

- amplitude;
- cycles;
- speed;
- decay.

Only their directions and initial phases differ.

Convenient symmetric directions corresponding to the vertices of a tetrahedron:

```cpp
{
    glm::normalize(glm::vec3{+1.0f, +1.0f, +1.0f}),
    glm::normalize(glm::vec3{-1.0f, -1.0f, +1.0f}),
    glm::normalize(glm::vec3{-1.0f, +1.0f, -1.0f}),
    glm::normalize(glm::vec3{+1.0f, -1.0f, -1.0f})
}
```

Phases:

```text
0
π / 2
π
3π / 2
```

Shader:

```glsl
const int maxWaves = 4;

uniform vec3 uSourceDirections[maxWaves];
uniform float uPhaseOffsets[maxWaves];

float evaluateWaveField(vec3 direction)
{
    float result = 0.0;

    for (int i = 0; i < maxWaves; ++i) {
        result += wave(
            direction,
            uSourceDirections[i],
            uPhaseOffsets[i]
        );
    }

    return result;
}
```

Wave superposition:

$$
h_{\text{total}}
=
\sum_{i=1}^{N} h_i
$$

This is what creates interference. No separate "interference formula" is required: the waves are simply added together.

Constructive interference occurs when the waves have the same sign and reinforce one another.

Destructive interference occurs when the waves have opposite signs and partially or completely cancel one another.

---

## 10. Pass the amplitude to the fragment shader

Vertex shader:

```glsl
out float vWaveHeight;
```

After evaluating the field:

```glsl
vWaveHeight = height;
```

Fragment shader:

```glsl
in float vWaveHeight;

out vec4 outColor;
```

The rasterizer automatically interpolates the value between the triangle's vertices.

The fragment shader does not receive the three original values directly. For each fragment, it receives the interpolated `vWaveHeight` value.

---

## 11. Create an unlit palette

For $N$ identical waves, the absolute value of the summed field is bounded by:

$$
\lvert h_{\text{total}} \rvert \leq N A
$$

Where:

- $N$ is the number of waves;
- $A$ is the amplitude of each wave.

Normalization:

```glsl
float maximumHeight =
    float(maxWaves) *
    max(uAmplitude, 0.0001);

float normalizedWave =
    clamp(
        vWaveHeight / maximumHeight,
        -1.0,
        1.0
    );
```

Three-color palette:

```glsl
vec3 negativeColor =
    vec3(0.12, 0.25, 1.00);

vec3 neutralColor =
    vec3(0.01, 0.01, 0.04);

vec3 positiveColor =
    vec3(1.00, 0.18, 0.65);

vec3 color;

if (normalizedWave < 0.0) {
    color = mix(
        neutralColor,
        negativeColor,
        -normalizedWave
    );
} else {
    color = mix(
        neutralColor,
        positiveColor,
        normalizedWave
    );
}

outColor = vec4(color, 1.0);
```

This is unlit: the color does not depend on a light, shadows, or a BRDF.

Positive and negative wave-field values simply receive different colors.

**Check:** interference nodes are nearly dark, while positive and negative extrema receive different colors.

---

## 12. Add minimal Dear ImGui controls

Only these controls:

```cpp
ImGui::SliderFloat(
    "Amplitude",
    &amplitude,
    0.0f,
    0.25f
);

ImGui::SliderFloat(
    "Cycles",
    &cycles,
    1.0f,
    20.0f
);

ImGui::SliderFloat(
    "Speed",
    &speed,
    0.0f,
    10.0f
);

ImGui::SliderFloat(
    "Decay",
    &decay,
    0.0f,
    2.0f
);

ImGui::Checkbox(
    "Pause",
    &paused
);

ImGui::Checkbox(
    "Wireframe",
    &wireframe
);
```

Wireframe:

```cpp
glPolygonMode(
    GL_FRONT_AND_BACK,
    wireframe
        ? GL_LINE
        : GL_FILL
);
```

Restore fill mode before rendering ImGui, or the interface will also become an avant-garde wire sculpture:

```cpp
glPolygonMode(
    GL_FRONT_AND_BACK,
    GL_FILL
);
```

Pausing must freeze the accumulated simulation time instead of resetting it.

One simple option:

```cpp
if (!paused) {
    simulationTime += deltaTime;
}
```

---

## 13. Add two small tests

### Icosphere size

```text
subdivision 0 → 12 vertices, 20 faces
subdivision 3 → 642 vertices, 1280 faces
subdivision 4 → 2562 vertices, 5120 faces
```

### Data validity

For every vertex:

```cpp
std::abs(
    glm::length(position) - 1.0f
) < epsilon
```

For every index:

```cpp
index < positions.size()
```

That is enough.

There is no need to test every pixel shade yet: the snapshot-test inquisition has not reached your sphere.

---

## 14. Final polish

- [ ] Enable back-face culling:

```cpp
glEnable(GL_CULL_FACE);
glCullFace(GL_BACK);
glFrontFace(GL_CCW);
```

- [ ] Add a parameter reset.
- [ ] Choose attractive initial values.
- [ ] Check that there are no OpenGL debug errors.
- [ ] Run the formatter.
- [ ] Run clang-tidy.
- [ ] Run the tests.
- [ ] Check the Debug build.
- [ ] Check the Release build.
- [ ] Record a short video or GIF.
- [ ] Add one screenshot to the README.

Good initial parameters:

```text
Amplitude: 0.12
Cycles:    8.0
Speed:     2.5
Decay:     0.35
Rotation:  0.15 rad/s
```

---

# Minimal File Structure

```text
src/
  main.cpp
  bootstrap/
    glfw_context.cpp
    glfw_context.hpp
  graphics/
    shader_program.cpp
    shader_program.hpp
    icosphere.cpp          # add when implementing the sphere
    icosphere.hpp
  ui/
    imgui_session.cpp
    imgui_session.hpp
  demo/
    triangle_demo.cpp      # remove after the static sphere appears
    triangle_demo.hpp
  support/
    app_options.hpp
    opengl_diagnostics.cpp
    opengl_diagnostics.hpp
    smoke_test.cpp
    smoke_test.hpp

shaders/
  wave.vert
  wave.frag

tests/
  app_options_test.cpp
  icosphere_test.cpp       # add together with the icosphere
```

Read the finished bootstrap in this order: `main.cpp` → `bootstrap/` →
`graphics/shader_program.*` → `ui/`. The `support/` directory can wait until later,
while `demo/triangle_demo.*` serves as a working reference until the first static
sphere is rendered and is then removed in its entirety.

Do not split shader uniforms, the camera, and the render loop into separate subsystems.

For a project this size, direct code in `main.cpp` is more honest and understandable than decorative architecture.

---

# Data Flow Between CPU and GPU

```text
CPU / C++
│
├── positions[]          → VBO
├── indices[]            → EBO
├── model/view/proj      → uniforms
├── source directions    → uniforms
├── amplitude/cycles     → uniforms
├── speed/decay/time     → uniforms
└── ImGui controls       → modify CPU parameters
                          ↓
Vertex shader
├── receives the direction
├── computes angular distance
├── sums the waves
├── changes the vertex radius
└── passes the wave height
                          ↓
Rasterizer
└── interpolates the wave height
                          ↓
Fragment shader
└── maps the wave height to color
```

---

# Mathematics Cheat Sheet

## Vector normalization

For a nonzero vector $\mathbf v$:

$$
\hat{\mathbf v}
=
\frac{\mathbf v}
{\lVert \mathbf v \rVert}
$$

```cpp
glm::normalize(v)
```

A normalized vector has a length of `1` and can represent a point on the unit sphere.

---

## Dot product

$$
\mathbf a \cdot \mathbf b
=
\lVert \mathbf a \rVert
\lVert \mathbf b \rVert
\cos \theta
$$

For unit vectors:

$$
\lVert \mathbf a \rVert
=
\lVert \mathbf b \rVert
=
1
$$

Therefore:

$$
\mathbf a \cdot \mathbf b
=
\cos \theta
$$

Consequently:

$$
\theta
=
\arccos
\left(
    \mathbf a \cdot \mathbf b
\right)
$$

---

## Distance along the sphere's surface

For a sphere with radius $R$, the arc length is:

$$
d = R\theta
$$

Our base sphere has a radius of `1`, so:

$$
d = \theta
$$

This is the geodesic distance along the surface, not the straight chord through the sphere.

---

## Harmonic wave

General form:

$$
h(x,t)
=
A
\sin
\left(
    kx - \omega t + \phi
\right)
$$

Where:

- $A$ is the amplitude;
- $k$ is the spatial frequency;
- $\omega$ is the angular velocity;
- $t$ is time;
- $\phi$ is the initial phase.

In this project, the angular distance $\theta$ plays the role of the coordinate $x$.

---

## Exponential decay

$$
e^{-D\theta}
$$

When $D = 0$:

$$
e^{-D\theta} = 1
$$

Therefore, the wave does not decay.

The larger $D$ is, the faster the amplitude decreases with distance from the source.

---

## Radial sphere deformation

$$
\mathbf p
=
\hat{\mathbf n}
(R + h)
$$

Where:

- $\hat{\mathbf n}$ is the vertex direction from the center;
- $R$ is the original radius;
- $h$ is the wave-field value.

We do not move the vertex separately along `x`, `y`, or `z`.

We change its distance from the center of the sphere.

---

## Superposition

$$
h_{\text{total}}
=
h_1 + h_2 + \dots + h_N
$$

Equivalent notation:

$$
h_{\text{total}}
=
\sum_{i=1}^{N} h_i
$$

Waves reinforce one another when their signs match.

Waves cancel one another when their signs are opposite.

---

# Troubleshooting Common Problems

## Black screen

Check:

1. shader compilation log;
2. shader linking log;
3. `glViewport`;
4. VAO binding;
5. index count;
6. camera position;
7. depth buffer clearing;
8. OpenGL debug callback.

---

## The inside of the sphere is visible

Temporarily disable culling.

Then check:

- the index order;
- `glFrontFace(GL_CCW)`;
- whether the model matrix reverses the orientation of the triangles.

---

## Holes or seams appear on the sphere

The midpoint is most likely being created separately for each adjacent triangle.

Check the edge cache.

Edges `(a, b)` and `(b, a)` must use the same key.

---

## The sphere disappears when parameters change

Check:

```glsl
clamp(
    dot(a, b),
    -1.0,
    1.0
)
```

It is also a good idea to maintain:

$$
A \cdot N < R
$$

Where:

- $A$ is the amplitude of one wave;
- $N$ is the number of waves;
- $R$ is the sphere radius.

For four waves and a unit radius, a safe starting point is:

```text
Amplitude ≤ 0.2
```

---

## The sphere looks angular

Increase the subdivision level from `3` to `4`.

Do not jump straight to `6`, because the problem is almost certainly not a shortage of triangles, although people have always enjoyed treating bugs with more data.

---

## The color has triangular boundaries

This is the result of interpolating `vWaveHeight` between vertices.

That is acceptable for the first version.

Later, the surface direction can be passed to the fragment shader and the field evaluated again there, but that is a separate improvement.

---

# Final Stopping Criterion

The project is finished when:

1. the sphere is generated procedurally;
2. the waves move and interfere;
3. the parameters can be changed through ImGui;
4. wireframe mode shows the actual geometry;
5. the README explains the `CPU → vertex shader → rasterizer → fragment shader` path;
6. the project builds without errors or warnings.

After that:

- [ ] make a commit;
- [ ] record the remaining ideas in `Future work`;
- [ ] stop development for the day.

Any sudden idea involving bloom, UBOs, an ECS, or a spectral solution to the Schrödinger equation belongs in `Future work`, not in the implementation.
