# Advanced Graphics Programming
## Authors
[Axel Alavedra Cabello](https://github.com/AxelAlavedra)

[Marc Guillen Salto](https://github.com/Marcgs96)

## Content
* Deferred Shading Implemented.
* G-Buffer with Position, Normal, Diffuse and Depth textures.
* Directional Light Pass rendering through a Quad.
* Point Light Pass through usage of Light Volumes with Spheres adjusted to radius of light.
* WASD for Camera Movement, Hold Right Click for Camera Rotation and Hold Shift for Camera "Sprint".
* Orbit around center of the scene using .
* Normal Mapping.
* Parallax Occlusion Mapping.

## Advanced Techniques Implemented
### Normal Mapping
The Normal Mapping we implemented only works for the Deferred Shading, if the user swaps to Forward Shading the diference can be easily observed.
The shader can be found inside the [shaders.glsl](https://github.com/ProboxAM/PGA-OpenGL/blob/main/Engine/WorkingDir/shaders.glsl) file under the name G_BUFFER_NORMAL_MAPPING.

![Comparison Image](/images/normalMapping.png)

### Parallax Occlusion Mapping
The Parallax Occlusion Mapping we implemented only works for the Deferred Shading, if the user swaps to Forward Shading the diference can be easily observed.
The shader can be found inside the [shaders.glsl](https://github.com/ProboxAM/PGA-OpenGL/blob/main/Engine/WorkingDir/shaders.glsl) file under the name RELIEF_MAPPING.

![Comparison Image](/images/parallaxOcclusionMapping.png)

## Repository
https://github.com/ProboxAM/PGA-OpenGL
