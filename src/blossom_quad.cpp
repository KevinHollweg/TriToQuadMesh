#include "blossom_quad.h"

using namespace std;
using namespace glm;


blossom_quad::blossom_quad() {}

blossom_quad::~blossom_quad() {}

PolyMesh workMesh;
std::vector<PolyMesh::VertexHandle> workMesh_Vertex_Vec(15);
//PolyMesh resultMesh_step1;

vector<PolyMesh::FaceHandle> faces_with_one_graph_edge;

bool debug = false;


void blossom_quad::do_blossom_algo() {
	// create work mesh, will later be changed
	create_test_mesh();

	// property for edge in connection graph
	OpenMesh::HPropHandleT<int>		hprop_graph_edge_idx;
	workMesh.add_property(hprop_graph_edge_idx, "hprop_graph_edge_idx");
	// property, if mesh edge has alredy edge in connection graph
	OpenMesh::HPropHandleT<int>		hprop_has_graph_edge;
	workMesh.add_property(hprop_has_graph_edge, "hprop_has_graph_edge");
	// property, if face was already used for quad generation
	OpenMesh::FPropHandleT<int>	fprop_graph_edge_count;
	workMesh.add_property(fprop_graph_edge_count, "fprop_graph_edge_count");

	// connectivity graph 
	graph *connection_graph = new graph();

	for (auto f_it = workMesh.faces_begin(); f_it != workMesh.faces_end(); ++f_it) {
		PolyMesh::FaceHandle fh = *f_it;
		int face_graph_edge_count = 0;
		int border_count = 0;
		// create graph from faces, neighbouring faces get an edge
		for (auto fh_it = workMesh.fh_iter(fh); fh_it.is_valid(); ++fh_it) {
			PolyMesh::HalfedgeHandle heh = *fh_it;
			PolyMesh::HalfedgeHandle oheh = workMesh.opposite_halfedge_handle(heh);
			PolyMesh::FaceHandle fhs = workMesh.face_handle(heh);
			PolyMesh::FaceHandle fhso = workMesh.opposite_face_handle(heh);

			if (fhso.idx() != -1 && workMesh.property(hprop_has_graph_edge, heh) == false) {
				graph_edge *internal_edge = new graph_edge();
				internal_edge->face0 = fhs;
				internal_edge->face1 = fhso;

				// calculate vertices for possible quad
				vector<PolyMesh::VertexHandle> quad_vertices = connect_triangles(fhs.idx(), fhso.idx());
				internal_edge->quad_vertices = quad_vertices;

				// calculate cost for graph edge
				float qual = quality_function(quad_vertices);
				internal_edge->cost = qual;

				// calculate, if edge is "bad"
				bool bad = is_bad_edge(workMesh.to_vertex_handle(heh), workMesh.from_vertex_handle(heh));
				if (bad) {
					internal_edge->cost = INTERNAL_BAD_EDGE_COST;
					internal_edge->bad = true;
					face_graph_edge_count--;
				}
				else {
					internal_edge->bad = false;
				}

				connection_graph->edges.push_back(*internal_edge);
				workMesh.property(hprop_graph_edge_idx, heh) = connection_graph->edges_counter;
				workMesh.property(hprop_graph_edge_idx, oheh) = connection_graph->edges_counter;
				connection_graph->edges_counter++;
				workMesh.property(hprop_has_graph_edge, heh) = true;
				workMesh.property(hprop_has_graph_edge, oheh) = true;
				face_graph_edge_count++;
			}
			else {
				border_count++;
			}
		}
		// add external edges
		if (border_count == 1) {
			for (auto fh_it = workMesh.fh_iter(fh); fh_it.is_valid(); ++fh_it) {
				PolyMesh::HalfedgeHandle heh = *fh_it;
				
			}
		}
		else if (border_count == 2) {
			for (auto fh_it = workMesh.fh_iter(fh); fh_it.is_valid(); ++fh_it) {
				
			}
		}
		
	
		workMesh.property(fprop_graph_edge_count, fh) = face_graph_edge_count;
		// check edge count of faces;
		if (workMesh.property(fprop_graph_edge_count, fh) == 0) {
			int unmarked_edges = 0;
			for (auto fh_it = workMesh.fh_iter(fh); fh_it.is_valid(); ++fh_it) {
				PolyMesh::HalfedgeHandle heh = *fh_it;
				PolyMesh::FaceHandle fhso = workMesh.opposite_face_handle(heh);
				if (fhso.idx() != -1) {
					connection_graph->edges[workMesh.property(hprop_graph_edge_idx, heh)].bad = false;
					unmarked_edges++;
				}
			}
			workMesh.property(fprop_graph_edge_count, fh) = unmarked_edges;
		}
		
		// save faces with only one edge
		if (workMesh.property(fprop_graph_edge_count, fh)) {
			faces_with_one_graph_edge.push_back(fh);
		}
	}

	// create step one result mesh

	connection_graph->print_graph();


}

void blossom_quad::create_test_mesh() {
	workMesh_Vertex_Vec[0] = workMesh.add_vertex(PolyMesh::Point(0, 0, 0));
	workMesh_Vertex_Vec[1] = workMesh.add_vertex(PolyMesh::Point(4, 0, 0));
	workMesh_Vertex_Vec[2] = workMesh.add_vertex(PolyMesh::Point(0, 2, 0));
	workMesh_Vertex_Vec[3] = workMesh.add_vertex(PolyMesh::Point(2, 2, 0));
	workMesh_Vertex_Vec[4] = workMesh.add_vertex(PolyMesh::Point(4, 2, 0));
	workMesh_Vertex_Vec[5] = workMesh.add_vertex(PolyMesh::Point(6, 2, 0));
	workMesh_Vertex_Vec[6] = workMesh.add_vertex(PolyMesh::Point(3, 3, 0));
	workMesh_Vertex_Vec[7] = workMesh.add_vertex(PolyMesh::Point(0, 4, 0));
	workMesh_Vertex_Vec[8] = workMesh.add_vertex(PolyMesh::Point(2, 4, 0));
	workMesh_Vertex_Vec[9] = workMesh.add_vertex(PolyMesh::Point(4, 4, 0));
	workMesh_Vertex_Vec[10] = workMesh.add_vertex(PolyMesh::Point(6, 4, 0));
	workMesh_Vertex_Vec[11] = workMesh.add_vertex(PolyMesh::Point(0, 6, 0));
	workMesh_Vertex_Vec[12] = workMesh.add_vertex(PolyMesh::Point(2, 6, 0));
	workMesh_Vertex_Vec[13] = workMesh.add_vertex(PolyMesh::Point(4, 6, 0));
	workMesh_Vertex_Vec[14] = workMesh.add_vertex(PolyMesh::Point(2, 8, 0));

	workMesh.add_face({ workMesh_Vertex_Vec[0], workMesh_Vertex_Vec[3], workMesh_Vertex_Vec[2] });
	workMesh.add_face({ workMesh_Vertex_Vec[3], workMesh_Vertex_Vec[1], workMesh_Vertex_Vec[4] });
	workMesh.add_face({ workMesh_Vertex_Vec[2], workMesh_Vertex_Vec[8], workMesh_Vertex_Vec[7] });
	workMesh.add_face({ workMesh_Vertex_Vec[2], workMesh_Vertex_Vec[3], workMesh_Vertex_Vec[8] });
	workMesh.add_face({ workMesh_Vertex_Vec[3], workMesh_Vertex_Vec[6], workMesh_Vertex_Vec[8] });
	workMesh.add_face({ workMesh_Vertex_Vec[3], workMesh_Vertex_Vec[4], workMesh_Vertex_Vec[6] });
	workMesh.add_face({ workMesh_Vertex_Vec[6], workMesh_Vertex_Vec[4], workMesh_Vertex_Vec[9] });
	workMesh.add_face({ workMesh_Vertex_Vec[6], workMesh_Vertex_Vec[9], workMesh_Vertex_Vec[8] });
	workMesh.add_face({ workMesh_Vertex_Vec[4], workMesh_Vertex_Vec[5], workMesh_Vertex_Vec[9] });
	workMesh.add_face({ workMesh_Vertex_Vec[7], workMesh_Vertex_Vec[12], workMesh_Vertex_Vec[11] });
	workMesh.add_face({ workMesh_Vertex_Vec[7], workMesh_Vertex_Vec[8], workMesh_Vertex_Vec[12] });
	workMesh.add_face({ workMesh_Vertex_Vec[8], workMesh_Vertex_Vec[13], workMesh_Vertex_Vec[12] });
	workMesh.add_face({ workMesh_Vertex_Vec[8], workMesh_Vertex_Vec[9], workMesh_Vertex_Vec[13] });
	workMesh.add_face({ workMesh_Vertex_Vec[9], workMesh_Vertex_Vec[10], workMesh_Vertex_Vec[13] });
	workMesh.add_face({ workMesh_Vertex_Vec[11], workMesh_Vertex_Vec[12], workMesh_Vertex_Vec[14] });
	workMesh.add_face({ workMesh_Vertex_Vec[12], workMesh_Vertex_Vec[13], workMesh_Vertex_Vec[14] });

	OpenMesh::IO::write_mesh(workMesh, "Created_Meshes/workMesh.off");
}

float blossom_quad::quality_function(std::vector<PolyMesh::VertexHandle> quad_vertices)
{
	vec3 *point0 = new vec3(workMesh.point(quad_vertices[0])[0], workMesh.point(quad_vertices[0])[1], workMesh.point(quad_vertices[0])[2]);
	vec3 *point1 = new vec3(workMesh.point(quad_vertices[1])[0], workMesh.point(quad_vertices[1])[1], workMesh.point(quad_vertices[1])[2]);
	vec3 *point2 = new vec3(workMesh.point(quad_vertices[2])[0], workMesh.point(quad_vertices[2])[1], workMesh.point(quad_vertices[2])[2]);
	vec3 *point3 = new vec3(workMesh.point(quad_vertices[3])[0], workMesh.point(quad_vertices[3])[1], workMesh.point(quad_vertices[3])[2]);

	vec3 edge0 = *point1 - *point0;
	vec3 edge1 = *point2 - *point1;
	vec3 edge2 = *point3 - *point2;
	vec3 edge3 = *point0 - *point3;

	//if (debug) {
	//	cout << "point0 " << point0->x << "," << point0->y << "," << point0->z << endl;
	//	cout << "point1 " << point1->x << "," << point1->y << "," << point1->z << endl;
	//	cout << "point2 " << point2->x << "," << point2->y << "," << point2->z << endl;
	//	cout << "point3 " << point3->x << "," << point3->y << "," << point3->z << endl;
	//	cout << "edge0 " << edge0.x << "," << edge0.y << "," << edge0.z << endl;
	//	cout << "edge1 " << edge1.x << "," << edge1.y << "," << edge1.z << endl;
	//	cout << "edge2 " << edge2.x << "," << edge2.y << "," << edge2.z << endl;
	//	cout << "edge3 " << edge3.x << "," << edge3.y << "," << edge3.z << endl;
	//}

	float PI = glm::pi<float>();
	vector<float> corners;
	// corner 0
	float scal0 = scalar_product(edge0, -edge3);	
	float bet0 = vec_length(edge0) * vec_length(edge3);	
	float temp0 = scal0 / bet0;
	float corner0 = glm::acos(std::min(std::max((double)temp0, -1.0), 1.0));
	corners.push_back((PI / 2.f - corner0));
	
	// corner 1
	float scal1 = scalar_product(edge1, -edge0);
	float bet1 = vec_length(edge1) * vec_length(edge0);
	//float corner1 = glm::acos(scal1 / bet1);
	float temp1 = scal1 / bet1;
	float corner1 = glm::acos(std::min(std::max((double)temp1, -1.0), 1.0));
	corners.push_back((PI / 2.f - corner1));

	// corner 2
	float scal2 = scalar_product(edge2, -edge1);
	float bet2 = vec_length(edge2) * vec_length(edge1);
	//float corner2 = glm::acos(scal2 / bet2);
	float temp2 = scal2 / bet2;
	float corner2 = glm::acos(std::min(std::max((double)temp2, -1.0), 1.0));
	corners.push_back((PI / 2.f - corner2));

	// corner 3
	float scal3 = scalar_product(edge3, -edge2);
	float bet3 = vec_length(edge3) * vec_length(edge2);
	//float corner3 = glm::acos(scal3 / bet3);
	float temp3 = scal3 / bet3;
	float corner3 = glm::acos(std::min(std::max((double)temp3, -1.0), 1.0));
	corners.push_back((PI / 2.f - corner3));

	//find largest corner
	float max_corner = *max_element(begin(corners), end(corners));
	float temp = 1.f - ((2.f / PI) * max_corner);
	float quality = 1 - std::max(temp, 0.f);
	return quality;
}

vector<PolyMesh::VertexHandle> blossom_quad::connect_triangles(int face0, int face1) {
	// join two faces into a quad face
	//cout << face0 << "; " << face1 << endl;
	//PolyMesh smallQuadMesh;
	vector<PolyMesh::VertexHandle> sqmVec(4);
	set<int> already_pushed_vertices;
	PolyMesh::FaceHandle fh0 = workMesh.face_handle(face0);
	PolyMesh::FaceHandle fh1 = workMesh.face_handle(face1);

	// push all vertices into the quad mesh
	//for (auto v_it = smallMesh.vertices_begin(); v_it != smallMesh.vertices_end(); ++v_it) {
	//	TriMesh::VertexHandle vh = *v_it;
	//	sqmVec[vh.idx()] = smallQuadMesh.add_vertex(smallMesh.point(vh));
	//	already_pushed_vertices.insert(vh.idx());
	//}

	vector<PolyMesh::VertexHandle> face_vhandles;
	// how many vertices of face 0 are already pushed
	int v_face_0 = 0;
	// the last pushed vertex of face 0
	int last_vert = 0;

	//cout << "first triangle" << endl;
	//go through first triangle, check every half edge
	for (auto fh_it = workMesh.fh_iter(fh0); fh_it.is_valid(); ++fh_it) {
		PolyMesh::HalfedgeHandle heh_t1 = *fh_it;
		//TriMesh::FaceHandle fhs = smallMesh.face_handle(heh);

		// push from vertex of active halfedge into vertex handle vector
		PolyMesh::VertexHandle fvhs = workMesh.from_vertex_handle(heh_t1);
		last_vert = fvhs.idx();
		//cout << last_vert << endl;
		face_vhandles.push_back(workMesh.vertex_handle(last_vert));
		v_face_0++;
		//cout << last_vert << endl;

		// check if active halfedge is connecting one, if yes, break
		PolyMesh::FaceHandle fhso = workMesh.opposite_face_handle(heh_t1);
		//cout << fhso.idx() << endl;
		if (fhso.idx() == face1) {
			break;
		}
		//TriMesh::VertexHandle fvhs = smallMesh.from_vertex_handle(heh);
		//TriMesh::VertexHandle tvhs = smallMesh.to_vertex_handle(heh);

	}
	//cout << "faces from first triangle" << v_face_0 << endl;

	//cout << "second triangle" << endl;
	//go through second triangle, check every half edge
	//cout << "last pushed vertex " << last_vert << endl;
	for (auto fh_it = workMesh.fh_iter(fh1); fh_it.is_valid(); ++fh_it) {
		PolyMesh::HalfedgeHandle heh_t2 = *fh_it;

		// check, whitch halfedge is active
		PolyMesh::VertexHandle fvhs = workMesh.from_vertex_handle(heh_t2);
		int akt_from_vert = fvhs.idx();
		PolyMesh::VertexHandle tvhs = workMesh.to_vertex_handle(heh_t2);
		int akt_to_vert = tvhs.idx();
		if (akt_from_vert == last_vert) {
			// if from vertex of active edge is already pushed, do nothing
			//cout << "if from" << endl;
			continue;
		}
		else if (akt_to_vert == last_vert) {
			// if to vertex of active edge is already pushed, do nothing
			//cout << "if to" << endl;
			continue;
		}
		else {
			// if neither to nor from vertex is alredy pushed, push from vertex
			face_vhandles.push_back(workMesh.vertex_handle(akt_from_vert));
			//cout << akt_from_vert << endl;
			//face_vhandles.push_back(smallMesh.vertex_handle(akt_from_vert));
		}
	}
	// push remaining vertices of first triangle 
	//cout << "last triangle" << endl;
	for (auto fh_it = workMesh.fh_iter(fh0); fh_it.is_valid(); ++fh_it) {
		PolyMesh::HalfedgeHandle heh = *fh_it;
		if (v_face_0 != 0) {
			// skip all already pushed vertices
			v_face_0--;
			continue;
		}
		else {
			PolyMesh::VertexHandle fvhs = workMesh.from_vertex_handle(heh);
			face_vhandles.push_back(workMesh.vertex_handle(fvhs.idx()));
			//cout << fvhs.idx() << endl;
		}
	}

	return face_vhandles;
}

bool blossom_quad::is_bad_edge(PolyMesh::VertexHandle vertex1, PolyMesh::VertexHandle vertex2) {
	vec3 *point0 = new vec3(workMesh.point(vertex1)[0], workMesh.point(vertex1)[1], workMesh.point(vertex1)[2]);
	vec3 *point1 = new vec3(workMesh.point(vertex2)[0], workMesh.point(vertex2)[1], workMesh.point(vertex2)[2]);
	float deltaX = point1->x - point0->x;
	float deltaY = point1->y - point0->y;
	if (deltaY == 0 || deltaX == 0) {
		return true;
	}
	else if (abs(deltaX) <= 0.1 * deltaY || abs(deltaY) <= 0.1 * deltaX) {
		return true;
	}
	else {
		return false;
	}
}

float blossom_quad::scalar_product(glm::vec3 vector1, vec3 vector2) {
	return (vector1[0] * vector2[0] + vector1[1] * vector2[1] + vector1[2] * vector2[2]);
}

float blossom_quad::vec_length(vec3 vector1) {
	return sqrt(((float)vector1[0]) * ((float)vector1[0]) + ((float)vector1[1]) * ((float)vector1[1]) + ((float)vector1[2]) * ((float)vector1[2]));
}