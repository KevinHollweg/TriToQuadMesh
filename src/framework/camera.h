#pragma once

#include "config.h"

class Camera {
public:
    Camera();

    ~Camera();

    void set(float upX, float upY, float upZ,
             float directionX, float directionY, float directionZ,
             float positionX, float positionY, float positionZ);

    void setPreset(int id);

    void lookAt(glm::vec3 eye, glm::vec3 center, glm::vec3 up);


    void reset();
    void setPreviousCameraFromCurrentCamera();
    void translate(float us, float vs, float ws);
    void rotateAroundU(float theta);
    void rotateAroundV(float theta);
    void rotateAroundW(float theta);

    glm::vec3 getViewUp() {
        //        return m_cameraViewUp;
        return glm::vec3(modelMatrix[1]);
    }

    glm::vec3 getViewPosition() {
        return m_cameraViewPosition;
    }


    glm::vec3 getViewDirection() {
        //        return m_cameraViewDirection;
        return glm::vec3(modelMatrix[2]);
    }

    const glm::mat4& getViewMatrix() {
        return viewMatrix;
    }

    const glm::mat4& getModelMatrix() {
        return modelMatrix;
    }

    const glm::mat4& getProjectionMatrix() {
        return projectionMatrix;
    }


    void storeCamera(std::string cameraFilename);
    void loadCamera(std::string cameraFilename);

    void update(float dt);

protected:
    void computeViewMatrix();
    void updateFromSDLKeyboard(float dt);
    void updateFromSDLMouse();
    //    glm::vec3 m_cameraViewUp; // Y
    //    glm::vec3 m_cameraViewDirection; // -Z

    glm::quat orientation;
    glm::vec3 m_cameraViewPosition; // P

    glm::mat4 viewMatrix;
    glm::mat4 modelMatrix;
    glm::mat4 projectionMatrix;

    float cameraSpeed = 1;
    float m_fovY;
    glm::vec2 prevMousePosition =  glm::vec2(0.0f, 0.0f);

};
