#include "Camera.h"
#include "math.h"

#define PI 3.14159265358979323846

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
{
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
{
    Position = glm::vec3(posX, posY, posZ);
    WorldUp = glm::vec3(upX, upY, upZ);
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix()
{
    if(Orbit)
        return glm::lookAt(Position, LookAt, Up);
    else
        return glm::lookAt(Position, Position + Front, Up);
}

void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime)
{
    float velocity = DoubleSpeed ? MovementSpeed * 4 * deltaTime : MovementSpeed * deltaTime;
    if (direction == Camera_Movement::FORWARD)
        Position += Front * velocity;
    if (direction == Camera_Movement::BACKWARD)
        Position -= Front * velocity;
    if (direction == Camera_Movement::LEFT)
        Position -= Right * velocity;
    if (direction == Camera_Movement::RIGHT)
        Position += Right * velocity;
    if (direction == Camera_Movement::UP)
        Position += Up * velocity;
}

void Camera::ProcessSpeed(bool doubleSpeed)
{
    DoubleSpeed = doubleSpeed;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch)
    {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    // update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
}

void Camera::ProcessArcBallMovement(float xoffset, float yoffset, float width, float height)
{
    glm::vec4 position(Position.x, Position.y, Position.z, 1);
    glm::vec4 pivot(LookAt.x, LookAt.y, LookAt.z, 1);

    float deltaAngleX = (2 * PI / width); // a movement from left to right = 2*PI = 360 deg
    float deltaAngleY = (PI / height);  // a movement from top to bottom = PI = 180 de

    xoffset *= deltaAngleX;
    yoffset *= deltaAngleY;

    glm::mat4x4 rotationMatrixX(1.0f);
    rotationMatrixX = glm::rotate(rotationMatrixX, xoffset, WorldUp);
    position = (rotationMatrixX * (position - pivot)) + pivot;

    glm::mat4x4 rotationMatrixY(1.0f);
    rotationMatrixY = glm::rotate(rotationMatrixY, yoffset, Right);
    glm::vec3 finalPosition = (rotationMatrixY * (position - pivot)) + pivot;

    Position = std::move(finalPosition);
    LookAt = std::move(LookAt);
    Front = glm::normalize(LookAt - Position);
    Right = glm::normalize(glm::cross(Front, WorldUp)); 
    Up = glm::normalize(glm::cross(Right, Front));
}

void Camera::ProcessMouseScroll(float yoffset)
{
    Zoom -= (float)yoffset;
    if (Zoom < 1.0f)
        Zoom = 1.0f;
    if (Zoom > 45.0f)
        Zoom = 45.0f;
}

void Camera::updateCameraVectors()
{
    // calculate the new Front vector
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    // also re-calculate the Right and Up vector
    Right = glm::normalize(glm::cross(Front, WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    Up = glm::normalize(glm::cross(Right, Front));
}


