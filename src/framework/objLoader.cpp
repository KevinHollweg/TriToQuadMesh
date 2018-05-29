/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "objLoader.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

using std::cout;
using std::endl;


inline std::istream& operator>>(std::istream& is, glm::vec2& v){
    is >> v.x >> v.y;
    return is;
}

inline std::istream& operator>>(std::istream& is, glm::vec3& v){
    is >> v.x >> v.y >> v.z;
    return is;
}

inline std::istream& operator>>(std::istream& is, glm::vec4& v){
    is >> v.x >> v.y >> v.z >> v.w;
    return is;
}


static bool openFile(std::ifstream& stream, const std::string &file, const std::string &baseFile)
{
    //first try absolute path
    stream.open(file,std::ios::in);
    if(stream.is_open())
    {
        return true;
    }

    //now try relative path from base file
    std::string parentDirectory = "";
    for(auto it = baseFile.rbegin() ; it != baseFile.rend(); ++it){
        if(*it == '/'){
            auto d = std::distance(it,baseFile.rend());
            parentDirectory = baseFile.substr(0,d);
        }
    }

    stream.open(parentDirectory + file,std::ios::in);
    if(stream.is_open())
    {
        return true;
    }

    return false;
}



bool ObjMaterialLibrary::load(const std::string &_file, const std::string &baseFile){
    file = _file;
    std::ifstream stream;


    if(!openFile(stream,file,baseFile))
    {
        cout << "ObjMaterialLoader: Could not open file " << file << endl;
        return false;
    }


    while(!stream.eof())
    {
        std::string line;
        std::getline(stream, line);

        //remove carriage return from windows
        line.erase( std::remove(line.begin(), line.end(), '\r'), line.end() );

        parseLine(line);
    }

#if 0
    cout << "Loaded " << materials.size() << " materials." << endl;
    for(auto& m : materials)
    {
        cout << m.name << endl;
    }
#endif

    return true;
}

ObjMaterial ObjMaterialLibrary::getMaterial(const std::string &name)
{
    for(ObjMaterial& m : materials){
        if(m.name == name)
            return m;
    }
    cout<<"Warning material '"<<name<<"' not found!"<<endl;
    return ObjMaterial("default");
}


void ObjMaterialLibrary::parseLine(const std::string &line)
{
    std::stringstream sstream(line);

    std::string header;
    sstream >> header;

    std::string rest;
    std::getline(sstream,rest);

    //remove first white space
    if(rest[0]==' ' && rest.size()>1)
    {
        rest = rest.substr(1);
    }

    std::stringstream restStream(rest);


    if(header == "newmtl")
    {
        ObjMaterial m(rest);
        materials.push_back(m);
        currentMaterial = &materials[materials.size()-1];
    }

    if(currentMaterial==nullptr)
        return;

    if(header == "Ns"){
        restStream >> currentMaterial->Ns;
    }else if(header == "Ni"){
        restStream >> currentMaterial->Ni;
    }else if(header == "d"){
        restStream >> currentMaterial->d;
    }else if(header == "Tr"){
        restStream >> currentMaterial->Tr;
    }else if(header == "Tf"){
        restStream >> currentMaterial->Tf;
    }else if(header == "illum"){
        restStream >> currentMaterial->illum;
    }else if(header == "Ka"){
        restStream >> currentMaterial->Ka;
    }else if(header == "Kd"){
        restStream >> currentMaterial->Kd;
        currentMaterial->color = currentMaterial->Kd;
    }else if(header == "Ks"){
        restStream >> currentMaterial->Ks;
    }else if(header == "Ke"){
        restStream >> currentMaterial->Ke;
    }
    //textures
    else if(header == "map_Ka"){
        restStream >> currentMaterial->map_Ka;
    }else if(header == "map_Kd"){
        restStream >> currentMaterial->map_Kd;
    }else if(header == "map_Ks"){
        restStream >> currentMaterial->map_Ks;
    }else if(header == "map_d"){
        restStream >> currentMaterial->map_d;
    }else if(header == "map_bump" || header == "bump"){
        restStream >> currentMaterial->map_bump;

    }
}

//====================================================================================================================

ObjLoader2::ObjLoader2(const std::string &file):file(file)
{
    loadFile(file);
}

bool ObjLoader2::loadFile(const std::string &_file)
{
    this->file = _file;

    std::ifstream stream(file, std::ios::in);
    if(!stream.is_open()) {
        return false;
    }

    cout << "objloader: loading file " << file << endl;


    while(!stream.eof()) {
        std::string line;
        std::getline(stream, line);
        //remove carriage return from windows
        line.erase( std::remove(line.begin(), line.end(), '\r'), line.end() );
        parseLine(line);
    }

    stream.close();

//    cout << "Vertices/Normals/TextureCoordinates: " << vertices.size() << "/" << normals.size() << "/" <<texCoords.size() << endl;
//    cout << "Triangles: " << faces.size() << endl;

    createVertexIndexList();
    separateVerticesByGroup();

    //TODO:
    //calculateMissingNormals();

    cout << "Vertices/Faces: " << outVertices.size() << "/" << outTriangles.size() << endl;
    return true;
}


void ObjLoader2::separateVerticesByGroup()
{
    //make sure faces from different triangle groups do not reference the same vertex
    //needs to be called after createVertexIndexList()

    std::vector<int> vertexReference(outVertices.size(),INVALID_VERTEX_ID);

    for(int t = 0; t < (int)triangleGroups.size() ; ++t){
        ObjTriangleGroup &tg = triangleGroups[t];
        for(int i = 0 ; i < tg.faces ; ++i){
            ObjTriangle &face = outTriangles[i+tg.startFace];

            for(int j = 0 ; j < 3 ; ++j){
                int index = face.v[j];
                if(vertexReference[index] == INVALID_VERTEX_ID)
                    vertexReference[index] = t;
                if(vertexReference[index] != t){
                    //duplicate vertices
                    VertexNT v = outVertices[index];
                    int newIndex = (int)outVertices.size();
                    outVertices.push_back(v);
                    face.v[j] = newIndex;
                    vertexReference.push_back(t);
                }
            }
        }
    }
}


void ObjLoader2::createVertexIndexList()
{
    std::vector<bool> vertices_used(vertices.size(),false);

    outVertices.resize(vertices.size());

    for(std::vector<IndexedVertex2> &f : faces){
        ObjTriangle fa;
        for(int i=0;i<3;i++){
            IndexedVertex2 &currentVertex = f[i];
            int vert = currentVertex.v;
            int norm = currentVertex.n;
            int tex = currentVertex.t;

            VertexNT verte;
            if(vert>=0){
                assert(vert < (int)vertices.size());
                verte.position = glm::vec4(vertices[vert],1);
            }
            if(norm>=0){
                assert(norm < (int)normals.size());
                verte.normal = glm::vec4(normals[norm],0);
            }
            if(tex>=0){
                assert(tex < (int)texCoords.size());
                verte.texture = texCoords[tex];
            }


            int index = INVALID_VERTEX_ID;
            if(vertices_used[vert]){
                if(verte==outVertices[vert]){
                    index = vert;
                }
            }else{
                outVertices[vert] = verte;
                index = vert;
                vertices_used[vert] = true;
            }

            if(index==INVALID_VERTEX_ID){
                index = (int)outVertices.size();
                outVertices.push_back(verte);
            }
            fa.v[i] = index;
        }

        outTriangles.push_back(fa);
    }
}

static std::vector<std::vector<IndexedVertex2> > triangulateFace(const std::vector<IndexedVertex2> &f)
{
    std::vector<std::vector<IndexedVertex2>> newFaces;

    //more than 3 indices -> triangulate
    std::vector<IndexedVertex2> face;
    int cornerCount = 1;
    IndexedVertex2 startVertex, lastVertex;
    for(const IndexedVertex2 &currentVertex : f){
        if(cornerCount<=3){
            if(cornerCount==1)
                startVertex = currentVertex;
            face.push_back(currentVertex);
            if(cornerCount==3)
                newFaces.push_back(face);
        }else{
            face.resize(3);
            face[0] = lastVertex;
            face[1] = currentVertex;
            face[2] = startVertex;
            newFaces.push_back(face);
        }

        lastVertex = currentVertex;
        cornerCount++;
    }
    return newFaces;
}


inline std::vector<std::string> split(const std::string &s, char delim) {
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> elems;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


//parsing index vertex
//examples:
//v1/vt1/vn1        12/51/1
//v1//vn1           51//4
static IndexedVertex2 parseIV(std::string &line)
{
    IndexedVertex2 iv;
    std::vector<std::string> s = split(line, '/');
    if(s.size()>0 && s[0].size()>0)
        iv.v = std::atoi(s[0].c_str()) - 1;
    if(s.size()>1 && s[1].size()>0)
        iv.t = std::atoi(s[1].c_str()) - 1;
    if(s.size()>2 && s[2].size()>0)
        iv.n = std::atoi(s[2].c_str()) - 1;
    return iv;
}


void ObjLoader2::parseLine(const std::string &line)
{
    std::stringstream sstream(line);

    std::string header;
    sstream >> header;

    std::string rest;
    std::getline(sstream,rest);

    //remove first white space
    if(rest[0]==' ' && rest.size()>1)
    {
        rest = rest.substr(1);
    }

    std::stringstream restStream(rest);

    if(header == "#")
    {
        //this is a comment - do nothing
    }
    else if(header == "mtllib")
    {
        //load material file
        materialLoader.load(rest,file);
    }
    else if(header == "usemtl")
    {
//        cout << "Use Material: " << rest << endl;
        //finish current group
        if(!triangleGroups.empty())
        {
            ObjTriangleGroup &currentGroup = triangleGroups[triangleGroups.size()-1];
			currentGroup.faces = (int)faces.size() - currentGroup.startFace;
        }
        //start new group with new material
        ObjTriangleGroup newGroup;
		newGroup.startFace = (int)faces.size();
        newGroup.material = materialLoader.getMaterial(rest);
        triangleGroups.push_back(newGroup);
    }
    else if(header == "v")
    {
        glm::vec3 v;
        restStream >> v;
        vertices.push_back(v);
    }
    else if(header == "vn")
    {
        glm::vec3 v;
        restStream >> v;
        normals.push_back(glm::normalize(v));
    }
    else if(header == "vt")
    {
        glm::vec2 v;
        restStream >> v;
        texCoords.push_back(v);
    }
    else if(header == "f")
    {
        std::vector<IndexedVertex2> ivs;
        std::string t;
        while(restStream >> t)
        {
            ivs.push_back(parseIV(t));
        }

        auto nf = triangulateFace(ivs);
        //relative indexing, when the index is negativ
        for(auto &f : nf)
        {
            for(auto &i : f)
            {
                if(i.v < 0 && i.v != INVALID_VERTEX_ID)
                {
					i.v = (int)vertices.size() + i.v + 1;
                }
                if(i.n < 0 && i.n != INVALID_VERTEX_ID)
                {
					i.n = (int)normals.size() + i.n + 1;
                }
                if(i.t < 0 && i.t != INVALID_VERTEX_ID)
                {
					i.t = (int)texCoords.size() + i.t + 1;
                }
            }
        }
        faces.insert(faces.end(),nf.begin(),nf.end());
    }
}

