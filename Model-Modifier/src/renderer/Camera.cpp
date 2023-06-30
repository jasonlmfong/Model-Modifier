#include "Camera.h"

Camera::Camera(glm::vec3 position, double yaw, double pitch)
	:m_CameraPosition(position)
{
    LookAt(yaw, pitch);

    SetViewMatrix();
}

Camera::~Camera()
{
}

void Camera::MoveCamera(glm::vec3 direction, float speed)
{
    m_CameraPosition += direction * speed;
}

void Camera::LookAt(double yaw, double pitch)
{
    m_CameraFront =
    {
        cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
        sin(glm::radians(pitch)),
        sin(glm::radians(yaw)) * cos(glm::radians(pitch))
    };
}

glm::vec3 Camera::GetCameraFront() const
{
    return m_CameraFront;
}

glm::vec3 Camera::GetCameraRight() const
{
    return m_CameraRight;
}

glm::vec3 Camera::GetCameraUp() const
{
    return m_CameraUp;
}

void Camera::SetViewMatrix()
{
    m_CameraRight = glm::normalize(glm::cross(m_WorldUp, m_CameraFront));

    m_CameraUp = glm::cross(m_CameraFront, m_CameraRight);

    m_ViewMatrix = glm::lookAt(m_CameraPosition, m_CameraPosition + m_CameraFront, m_CameraUp);
}

glm::mat4 Camera::GetViewMatrix() const
{
    return m_ViewMatrix;
}