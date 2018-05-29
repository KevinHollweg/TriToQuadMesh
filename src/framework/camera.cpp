#include "camera.h"

Camera::Camera() {
    reset();
}

Camera::~Camera() {

}



void Camera::reset() {
    orientation = glm::quat();
    m_cameraViewPosition = glm::vec3(0.0f, 0.0f, 4.0f);
    projectionMatrix = glm::perspective(glm::radians(70.0f),16.0f / 9.0f, 0.1f,1000.0f);
    computeViewMatrix();

}

void Camera::lookAt(glm::vec3 eye, glm::vec3 center, glm::vec3 up)
{
    viewMatrix = glm::lookAt(eye,center,up);
    modelMatrix = glm::inverse(viewMatrix);

    glm::mat3 rot = glm::mat3(modelMatrix);
    orientation = normalize(glm::quat_cast(rot));

    m_cameraViewPosition = eye;
}


void Camera::translate(float us, float vs, float ws)
{
    auto d2 = orientation*glm::vec3(-us,-vs,-ws);
    m_cameraViewPosition += d2;
}


void Camera::update(float dt)
{
    updateFromSDLKeyboard(dt);
    updateFromSDLMouse();
    computeViewMatrix();
}

void Camera::computeViewMatrix()
{
    glm::mat4 T = glm::translate(glm::mat4(1),glm::vec3(m_cameraViewPosition));
    glm::mat4 R = glm::mat4_cast(orientation);
    //        mat4 S = glm::scale(mat4(1),vec3(s));
    modelMatrix = T * R;
    //    modelMatrix = glm::mat4_cast(orientation) * glm::translate(glm::mat4(1),m_cameraViewPosition);
    viewMatrix = inverse(modelMatrix);
}

void Camera::updateFromSDLKeyboard(float dt)
{
    const Uint8 *keyBoardState = SDL_GetKeyboardState(NULL);
    float timeDelta = dt;
    float translateDelta = cameraSpeed * timeDelta * 10.0f;

    if (keyBoardState[SDL_SCANCODE_R]) { reset(); cameraSpeed = 1.0f;};
    if (keyBoardState[SDL_SCANCODE_W]) translate(0.0f, 0.0f, +translateDelta);
    if (keyBoardState[SDL_SCANCODE_S]) translate(0.0f, 0.0f, -translateDelta);
    if (keyBoardState[SDL_SCANCODE_A]) translate(+translateDelta, 0.0f, 0.0f);
    if (keyBoardState[SDL_SCANCODE_D]) translate(-translateDelta, 0.0f, 0.0f);
    if (keyBoardState[SDL_SCANCODE_Q]) translate(0.0f, +translateDelta, 0.0f);
    if (keyBoardState[SDL_SCANCODE_E]) translate(0.0f, -translateDelta, 0.0f);
}

void Camera::updateFromSDLMouse()
{
    int mouseX, mouseY;
    Uint32 buttons = SDL_GetMouseState(&mouseX, &mouseY);

    glm::vec2 newMousePos = glm::vec2(mouseX,mouseY);
    glm::vec2 relMovement = prevMousePosition - newMousePos;

    if ( SDL_BUTTON(SDL_BUTTON_LEFT) & buttons)
    {
        float thetaX = relMovement.x / 360.0f;
        float thetaY = relMovement.y / 360.0f;

        this->orientation = glm::rotate(this->orientation,thetaX,glm::vec3(0,1,0));
        this->orientation = glm::rotate(this->orientation,thetaY,glm::vec3(1,0,0));
        this->orientation = normalize(this->orientation);
    }
    prevMousePosition = newMousePos;
}

