#include "Camera.h"
#include "Window.h"
#include <cmath>

void Camera::UpdateCamera(float dt)
{
	autoPilot = Window::GetKeyboard()->KeyTriggered(KEYBOARD_Q) ? !autoPilot : autoPilot;
	if (autoPilot) {
		GotoNextStation(dt);
	}
	else {
		pitch -= (Window::GetMouse()->GetRelativePosition().y);
		yaw -= (Window::GetMouse()->GetRelativePosition().x);

		pitch = std::min(pitch, 90.0f);
		pitch = std::max(pitch, -90.0f);

		yaw = yaw < 0 ? yaw + 360.0f : yaw;
		yaw = yaw > 360 ? yaw - 360.0f : yaw;

		Matrix4 rotation = Matrix4::Rotation(yaw, Vector3(0, 1, 0));
		Vector3 forward = rotation * Vector3(0, 0, -1);
		Vector3 right = rotation * Vector3(1, 0, 0);

		float speed = 300.0f * dt;
		position = Window::GetKeyboard()->KeyDown(KEYBOARD_W) ? position + forward * speed : position;
		position = Window::GetKeyboard()->KeyDown(KEYBOARD_S) ? position - forward * speed : position;
		position = Window::GetKeyboard()->KeyDown(KEYBOARD_D) ? position + right * speed : position;
		position = Window::GetKeyboard()->KeyDown(KEYBOARD_A) ? position - right * speed : position;
		position.y = Window::GetKeyboard()->KeyDown(KEYBOARD_SPACE) ? position.y + speed : position.y;
		position.y = Window::GetKeyboard()->KeyDown(KEYBOARD_CONTROL) ? position.y - speed : position.y;
	}
}

Matrix4 Camera::BuildMatrixView()
{
	return Matrix4::Rotation(-pitch, Vector3(1, 0, 0)) *
		Matrix4::Rotation(-yaw, Vector3(0, 1, 0)) *
		Matrix4::Translation(-position);
}

void Camera::GotoNextStation(float dt)
{
	float t = (elapsed >= 5.0) ? 1.0 : elapsed / 5.0;
	int curr = stationNum;
	int next = (stationNum + 1 >= rail.size()) ? 0 : stationNum + 1;
	Station currStation = rail[curr];
	Station nextStation = rail[next];
	position = Vector3(lerp(currStation.pos.x, nextStation.pos.x, t), lerp(currStation.pos.y, nextStation.pos.y, t), lerp(currStation.pos.z, nextStation.pos.z, t));
	yaw = lerp(currStation.yaw, nextStation.yaw, t);
	pitch = lerp(currStation.pitch, nextStation.pitch, t);
	if (t == 1.0) {
		elapsed = 0;
		stationNum = next;
	}
	elapsed += dt;
	
}
