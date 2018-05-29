#pragma once
#include <stdio.h>
#include <vector>
#include <OpenMesh\Core\IO\MeshIO.hh>
#include <OpenMesh\Core\Mesh\PolyMesh_ArrayKernelT.hh>
#include <OpenMesh\Core\Mesh\TriMesh_ArrayKernelT.hh>
#include <OpenMesh\Core\System\config.h>
#include <OpenMesh\Core\Mesh\Status.hh>

typedef OpenMesh::TriMesh_ArrayKernelT<> TriMesh;
typedef OpenMesh::PolyMesh_ArrayKernelT<> PolyMesh;

class graph_edge {
public:
	graph_edge();
	~graph_edge();

	PolyMesh::FaceHandle face0;
	PolyMesh::FaceHandle face1;

	std::vector<PolyMesh::VertexHandle> quad_vertices;

	float cost;
	bool bad = false;

	bool to_be_used = false;
};


class graph {
public:
	graph();
	~graph();

	void print_graph();

	std::vector<graph_edge> edges;
	std::vector<graph_edge> external_edges;
	
	int edges_counter = 0;
	int external_edges_counter = 0;
};

