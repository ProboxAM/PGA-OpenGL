# Advanced Graphics Programming
## Authors
[Axel Alavedra Cabello](https://github.com/AxelAlavedra)

[Marc Guillen Salto](https://github.com/Marcgs96)

## Content
* Deferred Shading and Forward Shading Implemented with a selectable to swap between them.
* G-Buffer with Position, Normal, Diffuse and Depth textures, selectable to render each different texture.
* Directional Light Pass rendering through a Quad.
* Point Light Pass through usage of Light Volumes with Spheres adjusted to radius of light.
* WASD for Camera Movement, Hold Right Click for Camera Rotation and Hold Shift for Camera "Sprint".
* Orbit around origin of the scene Holding Left Click.
* Normal Mapping.
* Parallax Occlusion Mapping.

## Advanced Techniques Implemented
### Normal Mapping
The Normal Mapping we implemented only works for the Deferred Shading, if the user swaps to Forward Shading the diference can be easily observed.
The shader can be found inside the [shaders.glsl](https://github.com/ProboxAM/PGA-OpenGL/blob/main/Engine/WorkingDir/shaders.glsl) file under the name G_BUFFER_NORMAL_MAPPING.

![Comparison Image](/images/normalMapping.JPG)

![Comparison Gif](/images/normalMapGif.gif)

### Parallax Occlusion Mapping
The Parallax Occlusion Mapping we implemented only works for the Deferred Shading, if the user swaps to Forward Shading the diference can be easily observed.
The shader can be found inside the [shaders.glsl](https://github.com/ProboxAM/PGA-OpenGL/blob/main/Engine/WorkingDir/shaders.glsl) file under the name RELIEF_MAPPING.

![Comparison Image](/images/pom.jpg)

![Comparison Gif](/images/pomGif.gif)

## Content
* Combo box to swap between Forward and Deferred Shading.
* Combo box to select what texture to render in Deferred Shading.
* Advanced Techniques Settings Window with settings for Parallax Occlusion Mapping

![Settings Image](/images/settings.jpg)

## Repository
https://github.com/ProboxAM/PGA-OpenGL
