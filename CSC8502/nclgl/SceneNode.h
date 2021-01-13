#pragma once

#include "Matrix4.h";
#include "Vector3.h";
#include "Vector4.h";
#include "Mesh.h";
#include <vector>;

class SceneNode
{
public:
	SceneNode(Mesh*m = NULL, Vector4 colour = Vector4(1,1,1,1));
	~SceneNode(void);

	float GetBoundingRadius() const { return boundingRadius; };
	void SetBoundingRadius(float r) { boundingRadius = r; };

	Shader* GetShader() const { return shader; };
	void SetShader(Shader* s) { shader = s; };

	float GetCameraDistance() const { return distFromCamera; };
	void SetCameraDistance(float d) { distFromCamera = d; };

	void SetTexture(GLuint tex) { texture = tex; };
	GLuint GetTexture() const { return texture; };

	static bool CompareByCameraDistance(SceneNode* a, SceneNode* b) {
		return (a->distFromCamera < b->distFromCamera);
	}

	void SetTransform(const Matrix4 matrix) { transform = matrix; };
	const Matrix4& GetTransform() const { return transform; };
	Matrix4 GetWorldTransform() const { return worldTransform; };

	Vector4 GetColour() { return colour; };
	void SetColour(Vector4 c) { colour = c; };

	Vector3 GetModelScale() const { return modelScale; };
	void SetModelScale(Vector3 s) { modelScale = s; };

	Mesh* GetMesh() const { return mesh; };
	void SetMesh(Mesh* m) { mesh = m; };

	bool GetAnimated() const { return animated; };
	void SetAnimated(bool a) { animated = a; };

	void AddChild(SceneNode* s);

	virtual void Update(float dt);
	virtual void Draw(const OGLRenderer& r);
	virtual void DrawSubMesh(const OGLRenderer& r, int sub);
	virtual void GenerateNormals(const OGLRenderer& r);
	virtual void GenerateTangents(const OGLRenderer& r);

	std::vector<SceneNode*>::const_iterator GetChildIteratorStart() { return children.begin(); };
	std::vector<SceneNode*>::const_iterator GetChildIteratorEnd() { return children.end(); };

protected:
	SceneNode* parent;
	Mesh* mesh;

	float boundingRadius;
	float distFromCamera;

	bool animated = false;

	Shader* shader;
	Matrix4 worldTransform;
	Matrix4 transform;
	Vector3 modelScale;
	Vector4 colour;
	GLuint texture;
	std::vector<SceneNode*> children;
};

