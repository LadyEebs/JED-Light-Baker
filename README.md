# JED-Light-Baker
JED/SED Light Baker Plugin. Bakes lighting using crude/simple GPU accelerated ray tracing (not RTX/HW tracing).

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
