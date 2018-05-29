#pragma once

#include "geometrical_classes.h"
//#include "helper.h"
#include <stdio.h>
#include <tchar.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <vector>
#include <algorithm>
#include <iostream>


#include <OpenMesh\Core\IO\MeshIO.hh>
#include <OpenMesh\Core\Mesh\PolyMesh_ArrayKernelT.hh>
#include <OpenMesh\Core\Mesh\TriMesh_ArrayKernelT.hh>
#include <OpenMesh\Core\System\config.h>
#include <OpenMesh\Core\Mesh\Status.hh>

typedef OpenMesh::TriMesh_ArrayKernelT<> TriMesh;
typedef OpenMesh::PolyMesh_ArrayKernelT<> PolyMesh;


class naiv {
public:
	naiv();
	~naiv();

	void do_naiv_algo();

	mesh triangle_mesh;
private:
	std::vector<PolyMesh::VertexHandle> connect_triangles(int face0, int face1);

	float quality_function(std::vector<PolyMesh::VertexHandle> quad_vertices);

	void refine_mesh();

	float scalar_product(glm::vec3 vector1, glm::vec3 vector2);

	float vec_length(glm::vec3 vector1);
};