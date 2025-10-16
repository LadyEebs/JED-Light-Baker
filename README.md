# JED-Light-Baker
JED/SED Light Baker Plugin. Bakes lighting using crude/simple GPU accelerated ray tracing (not RTX/HW tracing). See bottom of readme for some image examples.

<img width="714" height="609" alt="image" src="https://github.com/user-attachments/assets/53a3d882-7c4b-4436-8728-292c1233545b" />

# Requirements
- JED/NJED/SED level editor of choice (note some of them have bugs with RGB lighting)
- A copy of Jedi Knight/Mysteries of the Sith
- A DX11 Capable PC

# Usage
## Sunlight
To create a sun, place a light and set flag "0x2" (sun flag). This light controls the sun direction, color and intensity.
Move it around the world origin (or anchor, see below) to control the direction of the sun light.
Range has no effect.

## Skylight
To create a sky light, place a light and set flag "0x4" (sky flag). This light controls the sky color and intensity.
Position and range have no effect.

## Anchor
To change the orbit position of the sun, place a light and set flag "0x8" (sun anchor). The sun light will then orbit this light instead of the world origin, useful for big off-center levels.
All settings except position do nothing.

# Features
- Directional sun light
- Point lights, both in the original JED style and a new more physically motivated style
- Sky lighting for natural outdoor ambient light
- Emissive surfaces (uses material fill color, note: 16 bit mats don't store this properly, use 8 bit for emissives or "Extra Light as Emissive")
- Indirect lighting with bounced light from both solid and translucent surfaces as well as translucent color tinting
- Gamma correct lighting (optional)
- Tone mapped result, if using very strong lights and aiming to avoid clamping to 1.0
- Smooth normals for curved surfaces

# How it Works
A barebones version of the level is upload to the GPU via Buffers/StructuredBuffers. This minimal version contains basic geometry (surface normal, vertex positions) and connectivity (adjoins).

Before lighting, dispatch over Vertex X Vertex and compare the distances and normals of each vertex, and sum up the opposing vertexes surface normal if we're below the threshold. The normal must be normalized when accessed (to avoid having to write another pass outright). This produces smooth normals on curved surfaces.

Then lighting is done in passes. For each pass (sun, direct lights, sky/emissive, and indirect lighting bounces), naively dispatch over Vertex X Ray (or Light) count, and cast a ray. Each ray starts at the current vertex sector, and either recurses into the next sector when an adjoin collision is found, or hits a surface and returns. Collisions are basic polygon/plane tests, like in JED, with adjoins taking precedent to avoid missing them for traversal (numerical precision can cause surfaces to be hit when a traversal would've been preferred). The resulting hit point is used to do shading, either by grabbing the sun or sky colors (if the hit was a sky marked surface), or surface colors (if it's a emissive or solid), depending on the pass. The result is atomically added to an accumulation buffer. In the case of the bounce light, the lighting result of the previous pass (not the accumulation, just the output of that specific pass) is also interpolated on the hit surface and used to modulate the surface color, propagating the light.

Sector and layer masking is done naively with a bitmask array, any vertex whos sector or layer is not marked in the bitmask is skipped in the shader. The "Build All" button simply fills all of the bits with 1s.

The results of the accumulation buffer are then read back from the GPU and sent back to JED. Sector ambient light is also updated.

<img width="1923" height="1288" alt="image" src="https://github.com/user-attachments/assets/1bbeb10d-d00b-48c8-875d-171cf8c6ce56" />
<img width="1923" height="1288" alt="image" src="https://github.com/user-attachments/assets/1f9f5559-9475-4c13-9eaf-4564cefa1dbf" />
<img width="1923" height="1288" alt="image" src="https://github.com/user-attachments/assets/8d3973c6-c6df-4389-a33a-98e10453e9b6" />
<img width="1923" height="1288" alt="image" src="https://github.com/user-attachments/assets/fb7ee629-3008-476e-8059-78d16e138e09" />
<img width="1923" height="1288" alt="image" src="https://github.com/user-attachments/assets/398777f6-ac9c-4826-bf69-8b3b85f175e4" />
<img width="1923" height="1288" alt="image" src="https://github.com/user-attachments/assets/2ac4de5a-275b-4ff7-a955-94badce3634c" />


