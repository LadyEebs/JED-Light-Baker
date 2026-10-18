# JED-Light-Baker
JED/SED Light Baker Plugin. Bakes lighting using crude/simple GPU accelerated ray tracing (not RTX/HW tracing). See bottom of readme for some image examples.

<img width="908" height="661" alt="image" src="https://github.com/user-attachments/assets/130f7d62-aebd-4ae4-8347-3b655391f091" />

# Requirements
- A Jed 0.951 compatible level editor (JED/ZEDNJED/SED)
- A copy of Jedi Knight/Mysteries of the Sith/Indiana Jones and the Infernal Machine
- A DX11 Capable PC

**Important** If you want to use RGB lighting, it's recommended to use a newer editor, like the latest NJed or SED, which fixes a bug in Jed's plugin system that caused lighting to turn teal.

# Download & Installation
[Download the latest](https://github.com/LadyEebs/JED-Light-Baker/releases/download/v2026.10.18.17/Light.Baker.2026-10-18-17.zip) or head over to the [Releases](https://github.com/LadyEebs/JED-Light-Baker/releases) page to download the version of your choosing.
Once downloaded, head over to your editor installs plugins directory (ex: jed/plugins/), and extract the dll from the zip into that directory.

If done successfully the Light Baker will show up in the list of plugins in Jed.

# Usage
## Sun Light (0x2)
To create a sun, place a light and set flag "0x2" (sun flag). This light controls the sun direction, color and intensity.
Move it around the world origin (or anchor, see below) to control the direction of the sun light.
Range has no effect.

## Sky Light (0x4)
To create a sky light, place a light and set flag "0x4" (sky flag). This light controls the sky color and intensity.
Position and range have no effect. When "Color from Material" is enabled, sky material albedos will be multiplied onto the sky light color.

## Anchor (0x8)
To change the orbit position of the sun, place a light and set flag "0x8" (sun anchor). The sun light will then orbit this light instead of the world origin, useful for big off-center levels.
All settings except position do nothing.

## Spot Light (0x10)

TODO: Not implemented yet.

## Ambient Light (0x20)
Ambient lights are multiplicative, meaning they darken or brighten the scene within their radius instead of adding light. They are useful for creating darkness for specific areas like pits or emphasizing points of interest.
The color and intensity of the ambient light is multiplied onto the existing light in the scene after the other lights have been applied.
To create an ambient light, place a light and set flag "0x20" (ambient flag). Position and range control the area of effect.

## Shadow Cleaving (0x40)
Set flag 0x40 (shadow cleaving flag) to enable "shadow casting" on a light.
This is a special flag that marks lights that should be used to cleave shadows, allowing finer control of what needs shadows and preventing unnecessary calculations.
Cleaving is only performed if the option is enabled in the baker. Use wisely, as this is destructive to the level geometry and potentially expensive to calculate.

# Features
- Directional sun light
- Point lights, both in the original JED style and a new more physically motivated style
- Sky lighting for natural outdoor ambient light
- Multiplicative ambient lights for darkening areas
- Soft shadows via ligher-style shadow cleaving
- Emissive surfaces (uses material fill color)
- Indirect lighting with bounced light from both solid and translucent surfaces as well as translucent color tinting
- Gamma correct lighting (optional)
- Tone mapped result, if using very strong lights and aiming to avoid clamping to 1.0 (or if you just want a punchier cinematic look)
- Smooth normals for curved surfaces

# Limitations
Materials currently rely on their solid fill color field to supply surface color for GI tracing.
For 8 bit materials, this is supported in Mat Master. For 16 bit or others, only Mat 16 1.4.3 and later support this. Older 16 bit mats will not generate bounced light or emission until resaved.

# How it Works
A barebones version of the level is uploaded to the GPU via Buffers/StructuredBuffers. This minimal version contains basic geometry (surface normal, vertex positions, fill colors) and connectivity (adjoins).

Before lighting, one or two passes are run over the geometry:
- If enabled, shadow cleaving is performed which slices up faces using clipping planes accumulated from the light through adjoins (or from sky surfaces in the case of the sun). This needs to run first since it modifies the geometry and the other passes rely on the modified geometry. This is the only pass done on the CPU because of the heavy geometry modifications and difficult to predict memory usage.
- A vertex adjacency graph is computed from the level geometry, using a spatial hash and a sparse matrix so we can smooth normals on the GPU, and thus lighting transitions, across surfaces and sector adjoins. This is done after cleaving since it modifies the geometry and would break the adjacency graph.

Then lighting is done in passes:
- For the sun, dispatch a thread for each vertex and trace a ray towards the sun. If no hit is found, the vertex color is set to the sun color. This is the first stage so it ignores the "previous" result and simply replaces the color.
- For direct lights, dispatch a group of threads per vertex, each thread processes a few lights depending on how many lights are in the scene, calculating lighting and accumulating the results locally and stored in groupshared memory, these are summed and the final result is written back to the vertex by the first thread.
- For sky/emissives, we do the same thing but each thread processes a few rays oriented around the hemisphere rather than lights, accumulating the sky color or emissive surface color from the hit.
- For indirect light we do the same as the sky except we ignore sky surfaces and modulate the surface color by the previous pass result. The shader does the same accumulation but also outputs the current result (not accumulated) for the next bounce. This propagates light across each bounce.
- Finally for the ambient pass, we simply multiply the current result by the ambient light color and intensity.

Tracing is straightforward: start at a sector, check the surfaces for intersection (starting with adjoins so that they take precedent in case of numerical precision issues). If there's a hit, recurse if the hit is an adjoin, or end the trace and return the hit surface index. Collisions are basic polygon/plane tests, like in JED. The resulting hit point is used to do shading, either by grabbing the sun or sky colors (if the hit was a sky marked surface), or surface colors (if it's a emissive or solid), depending on the pass.

Sector and layer masking is done naively with a bitmask array, any vertex whos sector or layer is not marked in the bitmask is skipped in the shader. The "Build All" button simply fills all of the bits with 1s.

The results of the accumulation buffer are then read back from the GPU and sent back to JED. Sector ambient light is also updated.

# Example Images
<img width="3840" height="2036" alt="image" src="https://github.com/user-attachments/assets/38061f79-2989-4c48-8003-937c78cc98b2" />
<img width="1916" height="1200" alt="image" src="https://github.com/user-attachments/assets/837f3498-3ae1-4fdc-96d3-bc08c5ffd743" />
<img width="1909" height="1191" alt="image" src="https://github.com/user-attachments/assets/ecf6c038-6d15-45a9-8a13-40cf080137c6" />
<img width="1919" height="1197" alt="image" src="https://github.com/user-attachments/assets/59b15e13-5285-4a48-8d8f-df678379de70" />
<img width="1923" height="1288" alt="image" src="https://github.com/user-attachments/assets/05fb9da6-3ed2-4050-8992-1faf8c547f3f" />
<img width="1923" height="1288" alt="image" src="https://github.com/user-attachments/assets/1bbeb10d-d00b-48c8-875d-171cf8c6ce56" />
<img width="1923" height="1288" alt="image" src="https://github.com/user-attachments/assets/1f9f5559-9475-4c13-9eaf-4564cefa1dbf" />
<img width="1923" height="1288" alt="image" src="https://github.com/user-attachments/assets/8d3973c6-c6df-4389-a33a-98e10453e9b6" />
<img width="1923" height="1288" alt="image" src="https://github.com/user-attachments/assets/fb7ee629-3008-476e-8059-78d16e138e09" />
<img width="1923" height="1288" alt="image" src="https://github.com/user-attachments/assets/398777f6-ac9c-4826-bf69-8b3b85f175e4" />
<img width="1923" height="1288" alt="image" src="https://github.com/user-attachments/assets/2ac4de5a-275b-4ff7-a955-94badce3634c" />


