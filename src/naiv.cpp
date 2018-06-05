#include "naiv.h"


using namespace std;
using namespace glm;


naiv::naiv() {
}

naiv::~naiv() {}



struct edge_with_value {
	TriMesh::HalfedgeHandle m_halfedge;
	int m_quality_value;
	vector<PolyMesh::VertexHandle> m_created_face;

	edge_with_value(PolyMesh::HalfedgeHandle e, vector<PolyMesh::VertexHandle> f, int q) :
		m_halfedge(e), 
		m_created_face(f),
		m_quality_value(q) 
		{}
	bool operator < (const edge_with_value& other_e) const {
		return (m_quality_value < other_e.m_quality_value);
	}
};

PolyMesh smallMesh;
PolyMesh resultMesh_step1;
PolyMesh resultMesh_refined;
std::vector<PolyMesh::VertexHandle> rvVec;

void naiv::do_naiv_algo() {
	//TriMesh smallMesh;
	std::vector<PolyMesh::VertexHandle> svVec(4);
	svVec[0] = smallMesh.add_vertex(PolyMesh::Point(0, 0, 0));
	svVec[1] = smallMesh.add_vertex(PolyMesh::Point(1, 0, 0));
	svVec[2] = smallMesh.add_vertex(PolyMesh::Point(0, 1, 0));
	svVec[3] = smallMesh.add_vertex(PolyMesh::Point(1, 1, 0));

	smallMesh.add_face({ svVec[0], svVec[1], svVec[3] });
	smallMesh.add_face({ svVec[0], svVec[3], svVec[2] });

	OpenMesh::IO::write_mesh(smallMesh, "Created_Meshes/smallMesh.off");

	// property, if face was already used for quad generation
	OpenMesh::FPropHandleT<bool>	fprop_connected_bool;
	smallMesh.add_property(fprop_connected_bool, "fprop_connected_bool");
	

	//smallMesh.face.add_property();

	set<int> connected_triangles;

	vector<edge_with_value> sorted_edges;

	/*
	 * create new mesh with all points from old mesh
	 */
	
	for (auto v_it = smallMesh.vertices_begin(); v_it != smallMesh.vertices_end(); ++v_it) {
		PolyMesh::VertexHandle vh = *v_it;
		rvVec.push_back(resultMesh_step1.add_vertex(smallMesh.point(vh)));
	}

	OpenMesh::IO::write_mesh(resultMesh_step1, "Created_Meshes/resultMesh.off");
	
	/*
	* go through all faces and for create a quad for every connected triangle
	* check the qualtiy of this connected quad and push the result into a vector
	*/
	for (auto f_it = smallMesh.faces_begin(); f_it != smallMesh.faces_end(); ++f_it) {
		PolyMesh::FaceHandle fh = *f_it;
		// set connected property to false
		smallMesh.property(fprop_connected_bool, fh) = false;

		for (auto fh_it = smallMesh.fh_iter(fh); fh_it.is_valid(); ++fh_it) {
			PolyMesh::HalfedgeHandle heh = *fh_it;
			PolyMesh::FaceHandle fhs = smallMesh.face_handle(heh);
			PolyMesh::FaceHandle fhso = smallMesh.opposite_face_handle(heh);
			//cout << "half edge: " << heh.idx() << ", half edge face: " << fhs.idx() << ", half edge opposite face: " << fhso.idx() << endl;
		
			// for every connected triangle
			if (fhso.idx() != -1) {
				// create quad
				vector<PolyMesh::VertexHandle> quad_vertices = connect_triangles(fhs.idx(), fhso.idx());
				
				// check qualtiy
				int qual = quality_function(quad_vertices);
				// push result into a vector
				edge_with_value *m_edge = new edge_with_value(heh, quad_vertices, qual);
				sorted_edges.push_back(*m_edge);
			}
		}
	}
	// sort result vector
	sort(sorted_edges.begin(), sorted_edges.end());

	/*
	 * go through sorted vector and create new faces, based on the results,
	 * only use triangle faces, that aren't already connected
	 */
	for (auto sort_ed = sorted_edges.begin(); sort_ed != sorted_edges.end(); ++sort_ed) {
		edge_with_value act_edge = *sort_ed;
		PolyMesh::FaceHandle fhs = smallMesh.face_handle(act_edge.m_halfedge);
		PolyMesh::FaceHandle fhso = smallMesh.opposite_face_handle(act_edge.m_halfedge);
		if ( (!smallMesh.property(fprop_connected_bool, fhs)) && (!smallMesh.property(fprop_connected_bool, fhso)) ) {
			/*
			 * add new face and mark used faces as connected
			 */
			smallMesh.property(fprop_connected_bool, fhs) = true;
			smallMesh.property(fprop_connected_bool, fhso) = true;
			//cout << act_edge.m_created_face[0] << ", " << act_edge.m_created_face[1] << ", " << act_edge.m_created_face[2] << ", " << act_edge.m_created_face[3] << endl;
			resultMesh_step1.add_face({ act_edge.m_created_face[0], act_edge.m_created_face[1], act_edge.m_created_face[2], act_edge.m_created_face[3] });

		}
		else {
			continue;
		}
	}

	/* 
	 * go through all faces, add not connected ones to new mesh;
	 */
	for (auto f_it = smallMesh.faces_begin(); f_it != smallMesh.faces_end(); ++f_it) {
		PolyMesh::FaceHandle fhs = *f_it;
		if (!smallMesh.property(fprop_connected_bool, fhs)) {
			vector<PolyMesh::VertexHandle> triVec;
			for (auto fv_it = smallMesh.fv_iter(fhs); fv_it.is_valid(); ++fv_it) {
				PolyMesh::VertexHandle vfhs = *fv_it;
				triVec.push_back(vfhs);
			}
			resultMesh_step1.add_face({ triVec[0], triVec [1], triVec [2]});
		}	
	}

	/*
	* go through all faces again and subdivide them into smaller quad faces
	*/
	refine_mesh();

	/*for (auto f_it = smallMesh.faces_begin(); f_it != smallMesh.faces_end(); ++f_it) {
		PolyMesh::FaceHandle fh = *f_it;
		cout << fh.idx() << endl;
	} */
	
	OpenMesh::IO::write_mesh(resultMesh_step1, "Created_Meshes/resultMesh.off");
	OpenMesh::IO::write_mesh(resultMesh_refined, "Created_Meshes/resultMesh_refined.off");
}

vector<PolyMesh::VertexHandle> naiv::connect_triangles(int face0, int face1) {
	// join two faces into a quad face
	//cout << face0 << "; " << face1 << endl;
	//PolyMesh smallQuadMesh;
	vector<PolyMesh::VertexHandle> sqmVec(4);
	set<int> already_pushed_vertices;
	PolyMesh::FaceHandle fh0 = smallMesh.face_handle(face0);
	PolyMesh::FaceHandle fh1 = smallMesh.face_handle(face1);

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
	for (auto fh_it = smallMesh.fh_iter(fh0); fh_it.is_valid(); ++fh_it) {
		PolyMesh::HalfedgeHandle heh_t1 = *fh_it;
		//TriMesh::FaceHandle fhs = smallMesh.face_handle(heh);

		// push from vertex of active halfedge into vertex handle vector
		PolyMesh::VertexHandle fvhs = smallMesh.from_vertex_handle(heh_t1);
		last_vert = fvhs.idx();
		//cout << last_vert << endl;
		face_vhandles.push_back(smallMesh.vertex_handle(last_vert));
		v_face_0++;
		//cout << last_vert << endl;

		// check if active halfedge is connecting one, if yes, break
		PolyMesh::FaceHandle fhso = smallMesh.opposite_face_handle(heh_t1);
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
	for (auto fh_it = smallMesh.fh_iter(fh1); fh_it.is_valid(); ++fh_it) {
		PolyMesh::HalfedgeHandle heh_t2 = *fh_it;

		// check, whitch halfedge is active
		PolyMesh::VertexHandle fvhs = smallMesh.from_vertex_handle(heh_t2);
		int akt_from_vert = fvhs.idx();
		PolyMesh::VertexHandle tvhs = smallMesh.to_vertex_handle(heh_t2);
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
			face_vhandles.push_back(smallMesh.vertex_handle(akt_from_vert));
			//cout << akt_from_vert << endl;
			//face_vhandles.push_back(smallMesh.vertex_handle(akt_from_vert));
		}
	}
	// push remaining vertices of first triangle 
	//cout << "last triangle" << endl;
	for (auto fh_it = smallMesh.fh_iter(fh0); fh_it.is_valid(); ++fh_it) {
		PolyMesh::HalfedgeHandle heh = *fh_it;
		if (v_face_0 != 0) {
			// skip all already pushed vertices
			v_face_0--;
			continue;
		}
		else {
			PolyMesh::VertexHandle fvhs = smallMesh.from_vertex_handle(heh);
			face_vhandles.push_back(smallMesh.vertex_handle(fvhs.idx()));
			//cout << fvhs.idx() << endl;
		}
	}

	return face_vhandles;
}

float naiv::quality_function(std::vector<PolyMesh::VertexHandle> quad_vertices)
{
	vec3 *point0 = new vec3(smallMesh.point(quad_vertices[0])[0], smallMesh.point(quad_vertices[0])[1], smallMesh.point(quad_vertices[0])[2]);
	vec3 *point1 = new vec3(smallMesh.point(quad_vertices[1])[0], smallMesh.point(quad_vertices[1])[1], smallMesh.point(quad_vertices[1])[2]);
	vec3 *point2 = new vec3(smallMesh.point(quad_vertices[2])[0], smallMesh.point(quad_vertices[2])[1], smallMesh.point(quad_vertices[2])[2]);
	vec3 *point3 = new vec3(smallMesh.point(quad_vertices[3])[0], smallMesh.point(quad_vertices[3])[1], smallMesh.point(quad_vertices[3])[2]);

	vec3 edge0 = *point1 - *point0;
	vec3 edge1 = *point2 - *point1;
	vec3 edge2 = *point3 - *point2;
	vec3 edge3 = *point0 - *point3;

	float PI = glm::pi<float>();
	vector<float> corners;
	// corner 0
	float scal0 = scalar_product(edge0, -edge3);
	float bet0 = vec_length(edge0) * vec_length(edge3);
	float corner0 = glm::acos(scal0 / bet0);
	corners.push_back((PI / 2.f - corner0));

	// corner 1
	float scal1 = scalar_product(edge1, -edge0);
	float bet1 = vec_length(edge1) * vec_length(edge0);
	float corner1 = glm::acos(scal1 / bet1);
	corners.push_back((PI / 2.f - corner1));

	// corner 2
	float scal2 = scalar_product(edge2, -edge1);
	float bet2 = vec_length(edge2) * vec_length(edge1);
	float corner2 = glm::acos(scal2 / bet2);
	corners.push_back((PI / 2.f - corner2));

	// corner 3
	float scal3 = scalar_product(edge3, -edge2);
	float bet3 = vec_length(edge3) * vec_length(edge2);
	float corner3 = glm::acos(scal3 / bet3);
	corners.push_back((PI / 2.f - corner3));

	//find largest corner
	float max_corner = *max_element(begin(corners), end(corners));
	float temp = 1.f - ((2.f / PI) * max_corner);
	float quality = 1 - std::max(temp, 0.f);
	return quality;
}

void naiv::refine_mesh() {

	int point_counter = 0;

	std::vector<PolyMesh::VertexHandle> resultVhVec;
	// copy all points to result mesh
	for (auto v_it = smallMesh.vertices_begin(); v_it != smallMesh.vertices_end(); ++v_it) {
		PolyMesh::VertexHandle vh = *v_it;
		resultVhVec.push_back(resultMesh_refined.add_vertex(smallMesh.point(vh)));
		point_counter++;
	}

	// property, edge mid point was already pushed
	OpenMesh::HPropHandleT<bool>	hprop_mid_found_bool;
	resultMesh_step1.add_property(hprop_mid_found_bool, "hprop_mid_found_bool");
	// property, index of edge mid point
	OpenMesh::HPropHandleT<int>		hprop_mid_idx_int;
	resultMesh_step1.add_property(hprop_mid_idx_int, "hprop_mid_idx_int");


	// TODO arrays!!!!
	PolyMesh::VertexHandle face_mid;
	vector<PolyMesh::VertexHandle> edge_mid(4);
	vector<PolyMesh::VertexHandle> edge_start(4);

	int face_mid_idx;
	vector<int> mid_idx(4);
	vector<int> start_idx(4);

	for (auto f_it = resultMesh_step1.faces_begin(); f_it != resultMesh_step1.faces_end(); ++f_it) {
		PolyMesh::FaceHandle fhs = *f_it;
		int edge_counter = 0;
		// vector with all four vertecies 
		vector<vec3> cornerPoints;

		// calculate mid point for every edge
		for (auto fh_it = resultMesh_step1.fh_iter(fhs); fh_it.is_valid(); ++fh_it) {
			PolyMesh::HalfedgeHandle heh = *fh_it;
			PolyMesh::HalfedgeHandle oheh = resultMesh_step1.opposite_halfedge_handle(heh);
			if (!resultMesh_step1.property(hprop_mid_found_bool, heh)) {
				resultMesh_step1.property(hprop_mid_found_bool, heh) = true;
				resultMesh_step1.property(hprop_mid_found_bool, oheh) = true;
				PolyMesh::VertexHandle idx_start = resultMesh_step1.from_vertex_handle(heh);
				PolyMesh::VertexHandle idx_end = resultMesh_step1.to_vertex_handle(heh);
				start_idx[edge_counter] = idx_start.idx();
				vec3 *start_point = new vec3(resultMesh_step1.point(idx_start)[0], resultMesh_step1.point(idx_start)[1], resultMesh_step1.point(idx_start)[2]);
				vec3 *end_point = new vec3(resultMesh_step1.point(idx_end)[0], resultMesh_step1.point(idx_end)[1], resultMesh_step1.point(idx_end)[2]);
				vec3 edge = (*end_point - *start_point);
				vec3 *mid_point = new vec3(start_point->x + 0.5 * edge.x, start_point->y + 0.5 * edge.y, start_point->z + 0.5 * edge.z);
				resultVhVec.push_back(resultMesh_refined.add_vertex(PolyMesh::Point(mid_point->x, mid_point->y, mid_point->z)));
				cornerPoints.push_back(*start_point);
				resultMesh_step1.property(hprop_mid_idx_int, heh) = point_counter;
				resultMesh_step1.property(hprop_mid_idx_int, oheh) = point_counter;
				mid_idx[edge_counter] = point_counter;
				point_counter++;
			}
			else {
				mid_idx[edge_counter] = resultMesh_step1.property(hprop_mid_idx_int, heh);
				PolyMesh::VertexHandle idx_start = resultMesh_step1.from_vertex_handle(heh);
				start_idx[edge_counter] = idx_start.idx();
			}
			edge_counter++;
		}
		// calculate face mid point
		if (edge_counter == 4) {
			float x = (1. / 4.) * (cornerPoints[0].x + cornerPoints[1].x + cornerPoints[2].x + cornerPoints[3].x);
			float y = (1. / 4.) * (cornerPoints[0].y + cornerPoints[1].y + cornerPoints[2].y + cornerPoints[3].y);
			float z = (1. / 4.) * (cornerPoints[0].z + cornerPoints[1].z + cornerPoints[2].z + cornerPoints[3].z);
			resultVhVec.push_back(resultMesh_refined.add_vertex(PolyMesh::Point(x,y,z)));
			face_mid_idx = point_counter;
			point_counter++;
		}
		else if (edge_counter == 3) {
			float x = (1. / 3.) * (cornerPoints[0].x + cornerPoints[1].x + cornerPoints[2].x);
			float y = (1. / 3.) * (cornerPoints[0].y + cornerPoints[1].y + cornerPoints[2].y);
			float z = (1. / 3.) * (cornerPoints[0].z + cornerPoints[1].z + cornerPoints[2].z);
			resultVhVec.push_back(resultMesh_refined.add_vertex(PolyMesh::Point(x, y, z)));
			face_mid_idx = point_counter;
			point_counter++;
		}


		// create new faces
		if (edge_counter == 4) {
			face_mid = resultVhVec[face_mid_idx];
			for (int k = 0; k < 4; k++) {
				edge_mid[k] = resultVhVec[mid_idx[k]];
				edge_start[k] = resultVhVec[start_idx[k]];
			}
			resultMesh_refined.add_face({ face_mid, edge_mid[0], edge_start[1], edge_mid[1] });
			resultMesh_refined.add_face({ face_mid, edge_mid[1], edge_start[2], edge_mid[2] });
			resultMesh_refined.add_face({ face_mid, edge_mid[2], edge_start[3], edge_mid[3] });
			resultMesh_refined.add_face({ face_mid, edge_mid[3], edge_start[0], edge_mid[0] });
		}
		else if (edge_counter == 3) {
			face_mid = resultVhVec[face_mid_idx];
			for (int k = 0; k < 4; k++) {
				edge_mid[k] = resultVhVec[mid_idx[k]];
				edge_start[k] = resultVhVec[start_idx[k]];
			}
			resultMesh_refined.add_face({ face_mid, edge_mid[0], edge_start[1], edge_mid[1] });
			resultMesh_refined.add_face({ face_mid, edge_mid[1], edge_start[2], edge_mid[2] });
			resultMesh_refined.add_face({ face_mid, edge_mid[2], edge_start[0], edge_mid[0] });
		}
	}
}

float naiv::scalar_product(glm::vec3 vector1, vec3 vector2) {
	return (vector1[0] * vector2[0] + vector1[1] * vector2[1] + vector1[2] * vector2[2]);
}

float naiv::vec_length(vec3 vector1) {
	return sqrt(vector1[0] * vector1[0] + vector1[1] * vector1[1] + vector1[2] * vector1[2]);
}
