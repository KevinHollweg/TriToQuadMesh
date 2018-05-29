#pragma once

#include "framework/window.h"
#include "framework/camera.h"
#include "framework/pngLoader.h"
#include "framework/objLoader.h"

#include <OpenMesh\Core\IO\MeshIO.hh>
#include <OpenMesh\Core\Mesh\PolyMesh_ArrayKernelT.hh>
#include <OpenMesh\Core\Mesh\TriMesh_ArrayKernelT.hh>

using namespace glm;

//#define SKELETON

struct Mesh
{
    GLuint vao, vbo, ibo;
    int numElements;
    GLenum drawMode;

    void render()
    {
        glBindVertexArray(vao);
        glDrawElements(drawMode,numElements,GL_UNSIGNED_INT,0);
    }
};


class CG : public Window
{
public:
    CG(int w, int h);
    virtual void update(float dt);
    virtual void render();
    virtual void renderGui();
	bool tested = false;
private:
    Camera camera;
    Mesh sphereMesh, ringMesh;
    float time = 0;
    float timeScale = 1;
    mat4 earth, moon, sun;
    mat4 earthOrbit, moonOrbit;


    //Solar System Paramters

    float sunRotationTime = 30;
    float sunObliquity = glm::radians(7.25f);
    float sunRadius = 1.5f;

    float earthRotationTime = 0.9972685185185185f;
    float earthRevolutionTime = 365.256f;
    float earthObliquity = glm::radians(23.4f);
    float earthRadius = 0.5f;
    float earthOrbitRadius = 5;

    float moonRevolutionTime = 27.323f;
    float moonRotationTime = moonRevolutionTime;
    float moonOrbitalInclination = glm::radians(5.14f);
    float moonObliquity = glm::radians(1.54f); //relative to ecliptic plane
    float moonRadius = 0.2f;
    float moonOrbitRadius = 1;
};
