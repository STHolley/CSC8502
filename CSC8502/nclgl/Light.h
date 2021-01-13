#pragma once
#include "Vector3.h"
#include "Vector4.h"

class Light
{
public:
	Light() {};
	Light(const Vector3& position, const Vector4& colour, float radius) {
		this->position = position;
		this->colour = colour;
		this->radius = radius;
	};

	~Light(void) {};

	Vector3 GetPosition() const { return position; };
	void SetPosition(const Vector3 p) { position = p; };

	Vector4 GetColour() const { return colour; };
	void SetColour(const Vector4 c) { colour = c; };

	float GetRadius() const { return radius; };
	void SetRadius(float r) { radius = r; };

protected:
	Vector3 position;
	Vector4 colour;
	float radius;
};

