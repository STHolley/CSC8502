#include "Renderer.h"
#include "..\nclgl\HeightMap.h"
#include "..\nclgl\Camera.h"
#include "..\nclgl\Light.h"
#include <algorithm>

const int LIGHT_NUM = 64;
#define SHADOWSIZE 16384
const int POST_PASSES = 10;

Renderer::Renderer(Window& w) : OGLRenderer(w) {
	//Load meshes
	sphere = Mesh::LoadFromMeshFile("Sphere.msh");
	cone = Mesh::LoadFromMeshFile("Cone.msh");
	cube = Mesh::LoadFromMeshFile("Cube.msh");
	quad = Mesh::GenerateQuad();
	heightMap = new HeightMap(TEXTUREDIR"noise.png");

	//Load Textures
	earthTex = SOIL_load_OGL_texture(TEXTUREDIR"cw/AsphaltDiffuse.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthBump = SOIL_load_OGL_texture(TEXTUREDIR"cw/AsphaltBump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	overgrowthTex = SOIL_load_OGL_texture(TEXTUREDIR"cw/GrassDiffuse.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	overgrowthBump = SOIL_load_OGL_texture(TEXTUREDIR"cw/GrassBump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	overgrowthMap = SOIL_load_OGL_texture(TEXTUREDIR"cw/GrassNoise.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	waterTex = SOIL_load_OGL_texture(TEXTUREDIR"cw/WaterDiffuse.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	waterBump = SOIL_load_OGL_texture(TEXTUREDIR"cw/WaterBump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	buildingTex = SOIL_load_OGL_texture(TEXTUREDIR"cw/BuildingDiffuse.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	buildingBump = SOIL_load_OGL_texture(TEXTUREDIR"cw/BuildingBump.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	buildingWindows = SOIL_load_OGL_texture(TEXTUREDIR"cw/BuildingWindow.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	cubeMap = SOIL_load_OGL_cubemap(TEXTUREDIR"cw/skybox/corona_rt.png", 
		TEXTUREDIR"cw/skybox/corona_lf.png", 
		TEXTUREDIR"cw/skybox/corona_up.png", 
		TEXTUREDIR"cw/skybox/corona_dn.png", 
		TEXTUREDIR"cw/skybox/corona_bk.png", 
		TEXTUREDIR"cw/skybox/corona_ft.png", 
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	if (!buildingTex || !buildingBump || !buildingWindows || !waterTex || !earthBump || !earthTex || !cubeMap || !waterBump || !overgrowthBump || !overgrowthMap || !overgrowthTex) {
		return;
	}

	SetTextureRepeating(earthTex, true);
	SetTextureRepeating(earthBump, true);
	SetTextureRepeating(overgrowthTex, true);
	SetTextureRepeating(overgrowthMap, true);
	SetTextureRepeating(waterTex, true);
	SetTextureRepeating(waterBump, true);
	SetTextureRepeating(buildingWindows, true);

	Vector3 heightMapSize = heightMap->GetHeightMapSize();

	camera = new Camera(0, 0, Vector3(heightMapSize.x / 2, 400, heightMapSize.z / 2));

	pointLights = new Light[LIGHT_NUM];
	srand(time(0));
	for (int i = 0; i < LIGHT_NUM; i++) {
		Light& l = pointLights[i];
		l.SetPosition(Vector3( rand() % (int)(heightMapSize.x), 300, rand() % (int)(heightMapSize.z)));
		l.SetColour(Vector4(1, 1, 1, 1));
		l.SetRadius(1000 + rand() % 250);
	}
	pointLights[0].SetPosition(camera->GetPosition());
	pointLights[0].SetColour(Vector4(1, 1, 1, 1));
	pointLights[0].SetRadius(10000);

	lightCycle.emplace_back(Vector3(2000, 2000, 2000));
	lightCycle.emplace_back(Vector3(5000, 0, 2000));
	lightCycle.emplace_back(Vector3(2000, -2000, 2000));
	lightCycle.emplace_back(Vector3(-1000, 0, 2000));
	

	//Load Shaders
	sceneShader = new Shader("cw/BumpVertex.glsl", "cw/BufferFragment.glsl");
	pointlightShader = new Shader("cw/PointLightVertex.glsl", "cw/PointLightFragment.glsl");
	combineShader = new Shader("cw/CombineVertex.glsl", "cw/CombineFragment.glsl");
	reflectShader = new Shader("cw/WaterVertex.glsl", "cw/WaterFragment.glsl");
	skyboxShader = new Shader("cw/SkyboxVertex.glsl", "cw/SkyboxFragment.glsl");
	shadowShader = new Shader("cw/ShadowVertex.glsl", "cw/ShadowFragment.glsl");
	processShader = new Shader("cw/TextureVertex.glsl", "cw/ProcessFragment.glsl");
	skeletonShader = new Shader("cw/SkinningVertex.glsl", "cw/SkinningFragment.glsl");

	if (!sceneShader->LoadSuccess() || !pointlightShader->LoadSuccess() || !combineShader->LoadSuccess() || !reflectShader->LoadSuccess() || !skyboxShader->LoadSuccess() || !shadowShader->LoadSuccess() || !processShader->LoadSuccess() || !skeletonShader->LoadSuccess()) {
		return;
	}

	glGenFramebuffers(1, &shadowFBO);
	glGenFramebuffers(1, &bufferFBO);
	glGenFramebuffers(1, &pointLightFBO);
	glGenFramebuffers(1, &postFBO);

	GLenum buffers[2] = {
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT1
	};

	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);


	GenerateScreenTexture(bufferDepthTex, true);
	for (int i = 0; i < 2; i++) {
		glGenTextures(1, &bufferColourTex[i]);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}
	GenerateScreenTexture(bufferNormalTex);
	GenerateScreenTexture(lightDiffuseTex);
	GenerateScreenTexture(lightSpecularTex);


	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
	glDrawBuffer(GL_NONE);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, bufferNormalTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glDrawBuffers(2, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lightDiffuseTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, lightSpecularTex, 0);
	glDrawBuffers(2, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	projMatrix = Matrix4::Perspective(1, 10000, (float)width / (float)height, 55);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	//Create scene node map
	actors = new SceneNode();

	for (int i = 0; i < buildingCount; i++) {
		SceneNode* s = new SceneNode();
		s->SetModelScale(Vector3(200 + (rand() % 800), 1500 + rand() % 1500, 200 + rand() % 800));
		s->SetMesh(cube);
		s->SetTransform(Matrix4::Translation(Vector3(rand() % (int)heightMapSize.x, s->GetModelScale().y / 2.0, rand() % (int)heightMapSize.z)) * Matrix4::Rotation(rand() % 360, Vector3(0, 1, 0)));
		s->SetBoundingRadius(1000);
		s->SetShader(sceneShader);
		actors->AddChild(s);
	}

	//Animated character man
	actor = Mesh::LoadFromMeshFile("Role_T.msh");
	anim = new MeshAnimation("Role_T.anm");
	material = new MeshMaterial("Role_T.mat");
	actor->GenerateNormals();
	actor->GenerateTangents();

	for (int i = 0; i < actor->GetSubMeshCount(); i++) {
		const MeshMaterialEntry* matEntry = material->GetMaterialForLayer(i);
		const string* filename = nullptr;
		matEntry->GetEntry("Diffuse", &filename);
		string path = TEXTUREDIR + *filename;
		GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
		matTextures.emplace_back(texID);
	}

	for (int i = 0; i < actorCount; i++) {
		SceneNode* s = new SceneNode();
		s->SetModelScale(Vector3(100, 100, 100));
		s->SetMesh(actor);
		s->SetTransform(Matrix4::Translation(Vector3(rand() % (int)heightMapSize.x, 150, rand() % (int)heightMapSize.z)) * Matrix4::Rotation(rand() % 360, Vector3(0, 1, 0)));
		s->GetTransform().GetPositionVector();
		s->SetAnimated(true);
		s->SetBoundingRadius(1);
		s->SetShader(skeletonShader);
		actors->AddChild(s);
	}

	waterRotate = 0;
	waterCycle = 0;
	currentFrame = 0;
	frameTime = 0.0f;
	stationNum = 0;
	init = true;
}

Renderer::~Renderer(void) {
	delete sceneShader;
	delete combineShader;
	delete pointlightShader;
	delete heightMap;
	delete camera;
	delete sphere;
	delete quad;
	delete reflectShader;
	delete skyboxShader;

	delete[] pointLights;

	glDeleteTextures(1, &shadowTex);
	glDeleteTextures(2, bufferColourTex);
	glDeleteTextures(1, &bufferNormalTex);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteTextures(1, &lightDiffuseTex);
	glDeleteTextures(1, &lightSpecularTex);
	glDeleteFramebuffers(1, &shadowFBO);
	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &pointLightFBO);
}

void Renderer::GenerateScreenTexture(GLuint& into, bool depth) {
	glGenTextures(1, &into);
	glBindTexture(GL_TEXTURE_2D, into);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	GLuint format = depth ? GL_DEPTH_COMPONENT24 : GL_RGBA8;
	GLuint type = depth ? GL_DEPTH_COMPONENT : GL_RGBA;

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, type, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildMatrixView();
	projMatrix = Matrix4::Perspective(1, 10000, (float)width / (float)height, 55);
	frameFrustum.FromMatrix(projMatrix * viewMatrix);
	projMatrix = Matrix4::Perspective(1, 10000, (float)width / (float)height, 55);
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_T)) {
		cycling = !cycling;
	}
	if (cycling) {
		float t = (elapsed >= 5.0) ? 1.0 : elapsed / 5.0;
		int curr = stationNum;
		int next = (stationNum + 1 >= lightCycle.size()) ? 0 : stationNum + 1;
		pointLights[0].SetRadius(10000);
		Vector3 currStation = lightCycle[curr];
		Vector3 nextStation = lightCycle[next];

		pointLights[0].SetPosition(Vector3(lerp(currStation.x, nextStation.x, t), lerp(currStation.y, nextStation.y, t), lerp(currStation.z, nextStation.z, t)));
		if (t == 1.0) {
			elapsed = 0;
			stationNum = next;
		}
		elapsed += dt;
	}

	waterRotate += dt * cos(dt);
	waterCycle += dt * sin(dt);

	frameTime -= dt;
	while (frameTime < 0.0f) {
		currentFrame = (currentFrame + 1) % anim->GetFrameCount();
		frameTime += 1.0f / anim->GetFrameRate();
	}

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_B)) {
		blur = !blur;
	}

	for (vector<SceneNode*>::const_iterator i = actors->GetChildIteratorStart(); i != actors->GetChildIteratorEnd(); i++) {
		if ((*i)->GetAnimated()) {
			if ((*i)->GetTransform().GetPositionVector().x < 0) {
				(*i)->SetTransform((*i)->GetTransform() * (Matrix4::Translation(Vector3(4000, 0, 0))));
			}
			if ((*i)->GetTransform().GetPositionVector().x > 4000) {
				(*i)->SetTransform((*i)->GetTransform() * (Matrix4::Translation(Vector3(-4000, 0, 0))));
			}
			if ((*i)->GetTransform().GetPositionVector().y < 0) {
				(*i)->SetTransform((*i)->GetTransform() * (Matrix4::Translation(Vector3(0, 0, 4000))));
			}
			if ((*i)->GetTransform().GetPositionVector().y > 4000) {
				(*i)->SetTransform((*i)->GetTransform() * (Matrix4::Translation(Vector3(0, 0, -4000))));
			}
			(*i)->SetTransform((*i)->GetTransform() * (Matrix4::Translation(Vector3(0, 0, 150) * dt)) * Matrix4::Rotation(20 * dt, Vector3(0, 1, 0)));
		}
	}
	actors->Update(dt);
}

void Renderer::RenderScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	BuildNodeLists(actors);
	DrawShadowScene();
	projMatrix = Matrix4::Perspective(1, 10000, (float)width / (float)height, 55);
	FillBuffers();
	DrawPointLights();
	if(blur) DrawPostProcess();
	CombineBuffers();
	ClearNodeLists();
}

void Renderer::DrawShadowScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	BindShader(shadowShader);
	
	viewMatrix = Matrix4::BuildViewMatrix(pointLights[0].GetPosition(), Vector3(2000, 0, 2000));
	projMatrix = Matrix4::Perspective(1, 10000, 1, 170);
	shadowMatrix = projMatrix * viewMatrix;
	UpdateShaderMatrices();
	heightMap->Draw();
	heightMap->Draw();
	DrawNodes(shadowShader);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::FillBuffers() {
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	viewMatrix = camera->BuildMatrixView();
	DrawSkybox();
	DrawGround();
	DrawActors();
	DrawWater();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawGround() {
	BindShader(sceneShader);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "bumpTex"), 1);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "mergeTex"), 2);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "mergeTex"), 3);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "stencilTex"), 4);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "shadowTex"), 5);

	glUniform3fv(glGetUniformLocation(sceneShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
	
	modelMatrix = Matrix4::Translation(Vector3(0,0,0));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, earthTex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, earthBump);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, overgrowthTex);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, overgrowthBump);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, overgrowthMap);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	SetShaderLight(pointLights[0]);
	UpdateShaderMatrices();

	heightMap->Draw();
}

void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);
	BindShader(skyboxShader);
	UpdateShaderMatrices();
	quad->Draw();
	glDepthMask(GL_TRUE);
}

void Renderer::DrawWater() {
	BindShader(reflectShader);

	SetShaderLight(pointLights[0]);
	glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "bumpTex"), 1);
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "cubeTex"), 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, waterTex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, waterBump);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	Vector3 hSize = heightMap->GetHeightMapSize();
	modelMatrix = Matrix4::Translation(hSize * 0.5) * Matrix4::Scale(hSize * 0.5) * Matrix4::Rotation(90, Vector3(1, 0, 0));
	textureMatrix = Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) * Matrix4::Scale(Vector3(10, 10, 10)) * Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));
	UpdateShaderMatrices();
	quad->Draw();
}

void Renderer::DrawActors() {
	DrawNodes();
}

void Renderer::BuildNodeLists(SceneNode* from) {
	if (frameFrustum.InsideFrustum(*from)) {
		Vector3 dir = from->GetWorldTransform().GetPositionVector() - camera->GetPosition();

		from->SetCameraDistance(Vector3::Dot(dir, dir));

		nodeList.push_back(from);
	}

	for (vector<SceneNode*>::const_iterator i = from->GetChildIteratorStart(); i != from->GetChildIteratorEnd(); i++) {
		BuildNodeLists(*i);
	}
}

void Renderer::DrawNodes(Shader* shader) {
	for (const auto& i : nodeList) {
		DrawNode(i, shader);
	}
}

void Renderer::DrawNode(SceneNode* n, Shader* shader) {
	if (n->GetMesh()) {
		shader = !shader ? n->GetShader() : shader;
		BindShader(shader);
		modelMatrix = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
		SetShaderLight(pointLights[0]);
		
		
		if (n->GetMesh()->GetSubMeshCount() > 1) {
			glUniform1i(glGetUniformLocation(shader->GetProgram(), "diffuseTex"), 0);
			glUniform1i(glGetUniformLocation(shader->GetProgram(), "bumpTex"), 1);
			glUniform1i(glGetUniformLocation(shader->GetProgram(), "shadowTex"), 2);
			vector<Matrix4> frameMatrices;
			for (unsigned int j = 0; j < actor->GetJointCount(); j++) {
				frameMatrices.emplace_back(anim->GetJointData(currentFrame)[j] * actor->GetInverseBindPose()[j]);
			}
			glUniformMatrix4fv(glGetUniformLocation(shader->GetProgram(), "joints"), frameMatrices.size(), false, (float*)frameMatrices.data());
			for (int j = 0; j < actor->GetSubMeshCount(); j++) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, matTextures[j]);
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, shadowTex);
				UpdateShaderMatrices();
				n->GetMesh()->DrawSubMesh(j);
			}
		}
		else {
			glUniform1i(glGetUniformLocation(shader->GetProgram(), "diffuseTex"), 0);
			glUniform1i(glGetUniformLocation(shader->GetProgram(), "bumpTex"), 1);
			glUniform1i(glGetUniformLocation(shader->GetProgram(), "mergeTex"), 2);
			glUniform1i(glGetUniformLocation(shader->GetProgram(), "mergeBump"), 3);
			glUniform1i(glGetUniformLocation(shader->GetProgram(), "stencilTex"), 4);
			glUniform1i(glGetUniformLocation(shader->GetProgram(), "shadowTex"), 5);
			glUniform3fv(glGetUniformLocation(shader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
			
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, buildingTex);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, buildingBump);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, waterTex);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, waterBump);
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, buildingWindows);
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_2D, shadowTex);
			UpdateShaderMatrices();
			n->Draw(*this);
		}
	}
}

void Renderer::ClearNodeLists() {
	nodeList.clear();
}

void Renderer::DrawPointLights() {
	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	BindShader(pointlightShader);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glBlendFunc(GL_ONE, GL_ONE);
	glCullFace(GL_FRONT);
	glDepthFunc(GL_ALWAYS);
	glDepthMask(GL_FALSE);

	glUniform1i(glGetUniformLocation(pointlightShader->GetProgram(), "depthTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);

	glUniform1i(glGetUniformLocation(pointlightShader->GetProgram(), "normTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bufferNormalTex);

	glUniform3fv(glGetUniformLocation(pointlightShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
	glUniform2f(glGetUniformLocation(pointlightShader->GetProgram(), "pixelSize"), 1.0 / width, 1.0 / height);

	Matrix4 invViewProj = (projMatrix * viewMatrix).Inverse();
	glUniformMatrix4fv(glGetUniformLocation(pointlightShader->GetProgram(), "inverseProjView"), 1, false, invViewProj.values);

	UpdateShaderMatrices();
	for (int i = 1; i < LIGHT_NUM; i++) {
		Light& l = pointLights[i];
		SetShaderLight(l);
		sphere->Draw();
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LEQUAL);

	glDepthMask(GL_TRUE);
	glClearColor(0.2, 0.2, 0.2, 1);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawPostProcess() {
	glBindFramebuffer(GL_FRAMEBUFFER, postFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(processShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	textureMatrix = Matrix4();
	UpdateShaderMatrices();
	glDisable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(processShader->GetProgram(), "sceneTex"), 0);
	for (int i = 0; i < POST_PASSES; ++i) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
		glUniform1i(glGetUniformLocation(processShader->GetProgram(), "isVertical"), 0);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
		quad->Draw();

		glUniform1i(glGetUniformLocation(processShader->GetProgram(), "isVertical"), 1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0], 0);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[1]);
		quad->Draw();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::CombineBuffers() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(combineShader);
	textureMatrix = Matrix4();
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, lightDiffuseTex);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, lightSpecularTex);
	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "diffuseLight"), 1);
	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "specularLight"), 2);
	

	quad->Draw();
}