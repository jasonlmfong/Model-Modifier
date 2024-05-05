#pragma once

#include "../external/glm/glm.hpp"
#include "../external/glm/trigonometric.hpp"
#include "../external/glm/gtc/matrix_transform.hpp"

class Camera
{
private:
	float m_Pitch, m_Yaw, m_Dist;

	glm::vec3 m_CameraPosition;
	glm::vec3 m_CameraFront;
	glm::vec3 m_CameraRight;
	glm::vec3 m_CameraUp;
	glm::vec3 m_WorldUp = { 0.0f, 1.0f, 0.0f };

	glm::mat4 m_ViewMatrix;

public:
	float m_FOV;

public:
	Camera(float pitch, float yaw, float distance);
	~Camera();

	void MoveCamera(glm::vec3 direction, float speed); 
	void RotateCamera(float deltaPitch, float deltaYaw, float deltaDist);
	void changeFOV(float scrollY);

	glm::vec3 GetPosOnSphere();
	void GetYawPitchDist();

	void ResetView();

	glm::vec3 GetCameraFront() const;
	glm::vec3 GetCameraRight() const;
	glm::vec3 GetCameraUp() const;

	void SetViewMatrix();
	glm::mat4 GetViewMatrix() const;
};