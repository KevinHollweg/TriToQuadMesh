#pragma once

#include "geometrical_classes.h"
#include "graph_classes.h"
#include <stdio.h>
#include <tchar.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <vector>
#include <algorithm>
#include <iostream>
#include <math.h>


#include <OpenMesh\Core\IO\MeshIO.hh>
#include <OpenMesh\Core\Mesh\PolyMesh_ArrayKernelT.hh>
#include <OpenMesh\Core\Mesh\TriMesh_ArrayKernelT.hh>
#include <OpenMesh\Core\System\config.h>
#include <OpenMesh\Core\Mesh\Status.hh>

typedef OpenMesh::TriMesh_ArrayKernelT<> TriMesh;
typedef OpenMesh::PolyMesh_ArrayKernelT<> PolyMesh;

#define EXTERNAL_EDGE_COST 10000.f
#define INTERNAL_BAD_EDGE_COST 5000.f



class blossom_quad {
public:
	blossom_quad();
	~blossom_quad();

	void do_blossom_algo();

	void create_test_mesh();
private:
	std::vector<PolyMesh::VertexHandle> connect_triangles(int face0, int face1);

	float quality_function(std::vector<PolyMesh::VertexHandle> quad_vertices);

	float scalar_product(glm::vec3 vector1, glm::vec3 vector2);

	float vec_length(glm::vec3 vector1);

	bool is_bad_edge(PolyMesh::VertexHandle vertex1, PolyMesh::VertexHandle vertex2);

	void remove_unnecessary_vertices();

	int remove_unnecessary_faces(int point_count);

	void refine_mesh();
};