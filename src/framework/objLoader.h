/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>



struct ObjMaterial{
    std::string name;
    glm::vec4 color = glm::vec4(0,1,0,0);

    float Ns = 0.2f;   //specular coefficient
    float Ni = 1;
    float d = 1;    //transparency
    float Tr = 1;
    glm::vec3 Tf;
    int illum ;
    glm::vec3 Ka = glm::vec3(0.2f);   //ambient color
    glm::vec4 Kd = glm::vec4(0.8f);   //diffuse color
    glm::vec3 Ks = glm::vec3(1);   //specular color
    glm::vec3 Ke = glm::vec3(1);

    std::string map_Ka;
    std::string map_Kd;
    std::string map_Ks;
    std::string map_d;
    std::string map_bump;

    ObjMaterial(const std::string& name=""):name(name){}
};



class ObjMaterialLibrary{
public:
    std::string file;
    bool verbose = false;
public:
    ObjMaterialLibrary(){}


    bool load(const std::string &file, const std::string &baseFile = "");

    ObjMaterial getMaterial(const std::string &name);
private:
    std::vector<ObjMaterial> materials;

    ObjMaterial* currentMaterial = nullptr;
    void parseLine(const std::string &line);

};



#define INVALID_VERTEX_ID -911365965
struct IndexedVertex2{
    int v=INVALID_VERTEX_ID;
    int n=INVALID_VERTEX_ID;
    int t=INVALID_VERTEX_ID;
};

struct VertexNT{
    glm::vec4 position = glm::vec4(0);
    glm::vec4 normal =  glm::vec4(0);
    glm::vec2 texture = glm::vec2(0);
    glm::vec2 padding;

    bool operator==(const VertexNT &other) const
    {
        return position == other.position && normal == other.normal && texture == other.texture;
    }
};

struct ObjTriangleGroup{
    int startFace = 0;
    int faces = 0;
    ObjMaterial material;
};

struct ObjTriangle{
    int v[3];
};

class ObjLoader2{
public:
    std::string file;
    bool verbose = false;

public:
    ObjLoader2(){}
    ObjLoader2(const std::string &file);

    bool loadFile(const std::string &file);

    std::vector<VertexNT> outVertices;
    std::vector<ObjTriangle> outTriangles;
    std::vector<ObjTriangleGroup> triangleGroups;
private:
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<std::vector<IndexedVertex2>> faces;
    ObjMaterialLibrary materialLoader;

    void createVertexIndexList();
    void separateVerticesByGroup();
    void calculateMissingNormals();

    void parseLine(const std::string &line);


};

