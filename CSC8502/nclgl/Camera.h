#pragma once
#include "Matrix4.h"
#include "Vector3.h"
#include <vector>

class Camera
{
public:
	Camera(float pitch = 0, float yaw = 0, Vector3 position = Vector3(0, 0, 0)) {
		this->pitch = pitch;
		this->yaw = yaw;
		this->position = position;
		this->stationNum = 0;
		this->autoPilot = false;
		this->elapsed = 0;
		AddStation(Station(Vector3(2000, 2000, 2000), -90, 0));
		AddStation(Station(Vector3(5000, 400, 2000), 0, 90));
		AddStation(Station(Vector3(5000, 400, 5000), 0, 45));
		AddStation(Station(Vector3(2000, 400, 5000), 0, 0));
		AddStation(Station(Vector3(-1000, 400, 5000), 0, -45));
		AddStation(Station(Vector3(-1000, 400, 2000), 0, -90));
		AddStation(Station(Vector3(-1000, 400, -1000), 0, -135));
		AddStation(Station(Vector3(2000, 400, -1000), 0, -180));
		AddStation(Station(Vector3(5000, 400, -1000), 0, -225));
	}
	~Camera(void) {};
	void UpdateCamera(float dt = 1.0f);

	Matrix4 BuildMatrixView();

	Vector3 GetPosition() const { return position; };
	void SetPosition(Vector3 v) { position = v; };

	float GetYaw() const { return yaw; };
	void SetYaw(float y) { yaw = y; };

	float GetPitch() const { return pitch; };
	void SetPitch(float p) { pitch = p; };

protected:
	struct Station {
		Vector3 pos;
		float pitch;
		float yaw;

		Station(Vector3 p = Vector3(0, 0, 0), float pi = 0, float ya = 0) {
			pos = p;
			pitch = pi;
			yaw = ya;
		};
	};

	float lerp(float a, float b, float t) { return a + t * (b - a); };
	void AddStation(Station s) {
		rail.push_back(s);
	}
	void GotoNextStation(float dt);

	float yaw;
	float pitch;
	Vector3 position;

	bool autoPilot;
	std::vector<Station> rail;
	int stationNum;
	float elapsed;
};

