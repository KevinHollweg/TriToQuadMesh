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

struct sort_edge {
	int m_orig_position;
	float m_cost_value;

	sort_edge(int o, float v) :
		m_orig_position(o),
		m_cost_value(v)
	{}
	bool operator < (const sort_edge& other_s) const {
		return (m_cost_value < other_s.m_cost_value);
	}
};

class graph_edge {
public:
	graph_edge();
	~graph_edge();

	PolyMesh::FaceHandle face0;
	PolyMesh::FaceHandle face1;

	std::vector<PolyMesh::VertexHandle> quad_vertices;
	PolyMesh::VertexHandle connecting_vertex;

	float cost;
	bool bad = false;

	bool to_be_used = false;
	bool use_edge_swap = false;
	bool use_vertex_duplication = false;
	bool no_longer_available = false;
};


class graph {
public:
	graph();
	~graph();

	void print_graph();
	void print_sorted_edges();

	std::vector<graph_edge> edges;
	std::vector<sort_edge> sorted_edges;
	std::vector<graph_edge> external_edges;
	
	int edges_counter = 0;
	int external_edges_counter = 0;
};

