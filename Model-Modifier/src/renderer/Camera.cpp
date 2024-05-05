#include "Camera.h"

Camera::Camera(float pitch, float yaw, float distance)
    : m_Pitch(pitch), m_Yaw(yaw), m_Dist(distance)
{
    ResetView();
}

Camera::~Camera()
{
}

void Camera::MoveCamera(glm::vec3 direction, float speed)
{
    m_CameraPosition += direction * speed;

    SetViewMatrix();
}

void Camera::RotateCamera(float deltaPitch, float deltaYaw, float deltaDist)
{
    GetYawPitchDist(); // set these properly first
    { // makes sure the pitch doesn't flip
        if (m_Pitch + deltaPitch > 1.5f)
            m_Pitch = 1.5f;
        else if (m_Pitch + deltaPitch < -1.5f)
            m_Pitch = -1.5f;
        else
            m_Pitch += deltaPitch;
    }
    m_Yaw += deltaYaw;
    m_Dist += deltaDist;

    glm::vec3 position = GetPosOnSphere();
    m_CameraPosition = position;
    m_CameraFront = -position;

    SetViewMatrix();
}

void Camera::changeFOV(float scrollY)
{
    m_FOV -= scrollY * 2.0f;
    m_FOV < 20.0f ? m_FOV = 20.0f : NULL;
    m_FOV > 110.0f ? m_FOV = 110.0f : NULL;
}

glm::vec3 Camera::GetPosOnSphere()
{
    return m_Dist * glm::vec3 
    {
        cos(m_Yaw) * cos(m_Pitch),
        sin(m_Pitch),
        sin(m_Yaw) * cos(m_Pitch)
    };
}

void Camera::GetYawPitchDist()
{
    m_Dist = glm::length(m_CameraPosition);
    glm::vec3 posSphere = m_CameraPosition / m_Dist;

    m_Pitch = glm::asin(posSphere.y);
    m_Yaw = glm::atan(posSphere.z, posSphere.x);
}

void Camera::ResetView()
{
    glm::vec3 defaultPos = { 0.0f, 1.25f, 2.5f };
    m_CameraPosition = defaultPos;
    m_CameraFront = -defaultPos; // look at the origin (0,0,0)
    m_FOV = 65;

    SetViewMatrix();
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