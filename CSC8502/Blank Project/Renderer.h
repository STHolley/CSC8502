#pragma once
#include "../nclgl/OGLRenderer.h"
#include "..\nclgl\SceneNode.h"
#include "..\nclgl\Frustum.h";
#include "..\nclgl\MeshAnimation.h"
#include "..\nclgl\MeshMaterial.h"
class Camera;
class Mesh;
class HeightMap;

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	~Renderer(void);

	void RenderScene() override;
	void UpdateScene(float dt) override;

protected:
	void DrawShadowScene();
	void FillBuffers();
	void DrawPointLights();
	void CombineBuffers();
	void GenerateScreenTexture(GLuint& into, bool depth = false);
	void DrawGround();
	void DrawWater();
	void DrawSkybox();
	void DrawActors();
	void DrawPostProcess();
	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes(Shader* shader = NULL);
	void DrawNode(SceneNode* n, Shader* shader = NULL);

	float lerp(float x, float y, float t) { return x + t * (y - x); };
	vector<Vector3> lightCycle;
	int stationNum = 0;
	float elapsed = 0;
	bool cycling = true;

	SceneNode* actors;
	Frustum frameFrustum;

	Shader* sceneShader; // Shader to fill our GBuffers
	Shader* pointlightShader; // Shader to calculate lighting
	Shader* combineShader; // shader to stick it all together
	Shader* reflectShader; // Skybox and light reflection shader
	Shader* skyboxShader; // Skybox shader
	Shader* shadowShader; // Real time shadow calculation mask
	Shader* shadowSceneShader; // Shader to combine for shadows
	Shader* processShader; // Post processing effects
	Shader* skeletonShader; // Skeleton animated character shader

	GLuint bufferFBO; // FBO for our G- Buffer pass
	GLuint pointLightFBO; // FBO for our lighting pass
	GLuint shadowFBO; // FBO for the real-time shadows
	GLuint postFBO; // FBO for the post-processing effects

	GLuint shadowTex; // Shadows go here
	GLuint bufferColourTex[2]; // Albedo goes here
	GLuint bufferNormalTex; // Normals go here
	GLuint bufferDepthTex; // Depth goes here
	GLuint lightDiffuseTex; // Store diffuse lighting
	GLuint lightSpecularTex; // Store specular lighting

	HeightMap* heightMap;
	Light* pointLights;

	Mesh* sphere;
	Mesh* cone;
	Mesh* quad;
	Mesh* cube;

	Camera* camera;
	Vector3 pointTo;
	
	Mesh* actor;
	float actorCount = 5;
	float buildingCount = 10;

	MeshAnimation* anim;
	MeshMaterial* material;
	vector<GLuint> matTextures;
	Matrix4 skeletonMat;

	GLuint cubeMap;

	GLuint earthTex;
	GLuint earthBump;
	GLuint overgrowthTex;
	GLuint overgrowthBump;
	GLuint overgrowthMap;
	GLuint buildingTex;
	GLuint buildingBump;
	GLuint buildingWindows;

	GLuint waterTex;
	GLuint waterBump;
	float waterRotate;
	float waterCycle;
	int currentFrame;
	float frameTime;

	bool blur = false;

	vector<SceneNode*> nodeList;
};