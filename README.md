# CSC8502

# Controls
Q: toggles between an automatic camera track which cycling around the scene and then doing a topdown view, and freelook  
(In Freelook)  
WASD: Move around the scene  
Space: Move up  
Shift: Move down  

T: Start and stop the main light track  
B: Toggle full screen gaussian blur  

# Scene contains the following features:  
Deferred rendering  
Multiple point lights  
Shadow mapping on whole scene + models  
Skeletal animation with shadow mapping and reflections to point lights  
All models minus the heightmap and water are in a scene node list  
Frustum culling for buildings and armoured people  
Texture stencils combining diffuse and normal maps  
Camera tracked skybox with accurate reflections in the water in the scene.  
Automated camera movement with rotation  
Whole screen gaussian blur (from tutorial)  
