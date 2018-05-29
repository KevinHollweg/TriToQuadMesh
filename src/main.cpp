#include "gui.h"
#include "geometrical_classes.h"
#include "blossom_quad.h"
#include "naiv.h"
#include "graph_classes.h"
//#include "helper.h"

#include <stdio.h>
#include <tchar.h>
#include <OpenMesh\Core\IO\MeshIO.hh>
#include <OpenMesh\Core\Mesh\PolyMesh_ArrayKernelT.hh>
#include <OpenMesh\Core\Mesh\TriMesh_ArrayKernelT.hh>

#undef main

typedef OpenMesh::TriMesh_ArrayKernelT<> TriMesh;
typedef OpenMesh::PolyMesh_ArrayKernelT<> PolyMesh;

using namespace std;

int main(int argc, char** argv) {

	TriMesh smallMesh;
	TriMesh midMesh;
	TriMesh largeMesh;

	std::vector<TriMesh::VertexHandle> svVec(4);
	std::vector<TriMesh::VertexHandle> mvVec(8);
	std::vector<TriMesh::VertexHandle> lvVec(13);

	svVec[0] = smallMesh.add_vertex(TriMesh::Point(0, 0, 0));
	svVec[1] = smallMesh.add_vertex(TriMesh::Point(1, 0, 0));
	svVec[2] = smallMesh.add_vertex(TriMesh::Point(0, 1, 0));
	svVec[3] = smallMesh.add_vertex(TriMesh::Point(1, 1, 0));

	smallMesh.add_face({ svVec[0], svVec[1], svVec[3] });
	smallMesh.add_face({ svVec[0], svVec[3], svVec[2] });

	//OpenMesh::IO::write_mesh(smallMesh, "Created_Meshes/smallMesh.off");

	mvVec[0] = midMesh.add_vertex(TriMesh::Point(1, 0, 0));
	mvVec[1] = midMesh.add_vertex(TriMesh::Point(1, 1, 0));
	mvVec[2] = midMesh.add_vertex(TriMesh::Point(2, 1, 0));
	mvVec[3] = midMesh.add_vertex(TriMesh::Point(3, 1, 0));
	mvVec[4] = midMesh.add_vertex(TriMesh::Point(0, 2, 0));
	mvVec[5] = midMesh.add_vertex(TriMesh::Point(1, 2, 0));
	mvVec[6] = midMesh.add_vertex(TriMesh::Point(2, 2, 0));
	mvVec[7] = midMesh.add_vertex(TriMesh::Point(1, 3, 0));

	midMesh.add_face({ mvVec[0], mvVec[2], mvVec[1] });
	midMesh.add_face({ mvVec[2], mvVec[3], mvVec[6] });
	midMesh.add_face({ mvVec[1], mvVec[2], mvVec[6] });
	midMesh.add_face({ mvVec[1], mvVec[6], mvVec[5] });
	midMesh.add_face({ mvVec[4], mvVec[5], mvVec[7] });
	midMesh.add_face({ mvVec[5], mvVec[6], mvVec[7] });

	//OpenMesh::IO::write_mesh(midMesh, "Created_Meshes/midMesh.off");

	lvVec[0] = largeMesh.add_vertex(TriMesh::Point(2, 0, 0));
	lvVec[1] = largeMesh.add_vertex(TriMesh::Point(4, 0, 0));
	lvVec[2] = largeMesh.add_vertex(TriMesh::Point(6, 0, 0));
	lvVec[3] = largeMesh.add_vertex(TriMesh::Point(5, 1, 0));
	lvVec[4] = largeMesh.add_vertex(TriMesh::Point(1, 2, 0));
	lvVec[5] = largeMesh.add_vertex(TriMesh::Point(2, 2, 0));
	lvVec[6] = largeMesh.add_vertex(TriMesh::Point(4, 2, 0));
	lvVec[7] = largeMesh.add_vertex(TriMesh::Point(6, 2, 0));
	lvVec[8] = largeMesh.add_vertex(TriMesh::Point(2, 4, 0));
	lvVec[9] = largeMesh.add_vertex(TriMesh::Point(4, 4, 0));
	lvVec[10] = largeMesh.add_vertex(TriMesh::Point(6, 4, 0));
	lvVec[11] = largeMesh.add_vertex(TriMesh::Point(2, 5, 0));
	lvVec[12] = largeMesh.add_vertex(TriMesh::Point(6, 5, 0));

	largeMesh.add_face({ lvVec[0], lvVec[5], lvVec[4] });
	largeMesh.add_face({ lvVec[0], lvVec[6], lvVec[5] });
	largeMesh.add_face({ lvVec[0], lvVec[1], lvVec[6] });
	largeMesh.add_face({ lvVec[1], lvVec[3], lvVec[6] });
	largeMesh.add_face({ lvVec[1], lvVec[2], lvVec[3] });
	largeMesh.add_face({ lvVec[2], lvVec[7], lvVec[3] });
	largeMesh.add_face({ lvVec[3], lvVec[7], lvVec[6] });
	largeMesh.add_face({ lvVec[4], lvVec[5], lvVec[8] });
	largeMesh.add_face({ lvVec[5], lvVec[6], lvVec[8] });
	largeMesh.add_face({ lvVec[6], lvVec[9], lvVec[8] });
	largeMesh.add_face({ lvVec[6], lvVec[10], lvVec[9] });
	largeMesh.add_face({ lvVec[6], lvVec[7], lvVec[10] });
	largeMesh.add_face({ lvVec[8], lvVec[9], lvVec[11] });
	largeMesh.add_face({ lvVec[9], lvVec[10], lvVec[12] });

	//OpenMesh::IO::write_mesh(largeMesh, "Created_Meshes/largeMesh.off");

	/*  for every face -> create triangles
	*		every vertex with index and position
	*		every halfedge and connected vertex
	*/
	//cout << "needed information per triangle:" << endl;
	for (auto f_it = smallMesh.faces_begin(); f_it != smallMesh.faces_end(); ++f_it) {
		TriMesh::FaceHandle fh = *f_it;
		//std::cout << "face: " << fh.idx() << std::endl;
		for (auto fh_it = smallMesh.fh_iter(fh); fh_it.is_valid(); ++fh_it) {
			TriMesh::HalfedgeHandle heh = *fh_it;
			TriMesh::FaceHandle fhs = smallMesh.face_handle(heh);
			TriMesh::FaceHandle fhso = smallMesh.opposite_face_handle(heh);
			//cout << "half edge: " << heh.idx() << ", half edge face: " << fhs.idx() << ", half edge opposite face: " << fhso.idx() << endl;
		}
		for (auto fv_it = smallMesh.fv_iter(fh); fv_it.is_valid(); ++fv_it) {
			TriMesh::VertexHandle vh = *fv_it;
			//cout << "vertex: " << vh.idx() << " ; coords: " << smallMesh.point(vh) << endl;
			//cout << "outgoing edges:" << endl;
			for (auto voh_it = smallMesh.voh_iter(vh); voh_it.is_valid(); ++voh_it) {
				TriMesh::HalfedgeHandle heh = *voh_it;
				//cout << heh.idx() << endl;
			}
		}
	}
	//cout << " " << endl;
	//cout << " " << endl;
	//cout << " " << endl;
	/*	for every vertex:
	*		every connected vertex
	*		every connected face
	*/
	//cout << "needed information per vertex:" << endl;
	for (auto v_it = smallMesh.vertices_begin(); v_it != smallMesh.vertices_end(); ++v_it) {
		TriMesh::VertexHandle vh = *v_it;
		//cout << "vertex " << vh.idx() << endl;
		for (auto vv_it = smallMesh.vv_iter(vh); vv_it.is_valid(); ++vv_it) {
			TriMesh::VertexHandle vvh = *vv_it;
			//cout << " has neighbor vertex: " << vvh.idx() << endl;
		}
		for (auto vf_it = smallMesh.vf_iter(vh); vf_it.is_valid(); ++vf_it) {
			TriMesh::FaceHandle vfh = *vf_it;
			//cout << " has neighbor face: " << vfh.idx() << endl;
		}
	}
	//cout << " " << endl;
	//cout << " " << endl;
	//cout << " " << endl;
	/*	for rendering the whole mesh:
	*		every vertex with its coordinates
	*		every face with its index
	*/
	//cout << "mesh data for rendering:" << endl;
	for (auto v_it = smallMesh.vertices_begin(); v_it != smallMesh.vertices_end(); ++v_it) {
		TriMesh::VertexHandle vh = *v_it;
		//cout << "vertex: " << vh.idx() << " ; coords: " << smallMesh.point(vh) << endl;
	}
	for (auto f_it = smallMesh.faces_begin(); f_it != smallMesh.faces_end(); ++f_it) {
		TriMesh::FaceHandle fh = *f_it;
		//std::cout << "face: " << fh.idx() << std::endl;
		for (auto fv_it = smallMesh.fv_iter(fh); fv_it.is_valid(); ++fv_it) {
			TriMesh::VertexHandle vh = *fv_it;
			//cout << "vertex: " << vh.idx() << endl;
		}
	}

	// join two faces into a quad face
	PolyMesh smallQuadMesh;
	int face0 = 0;
	int face1 = 1;
	std::vector<PolyMesh::VertexHandle> sqmVec(4);
	set<int> already_pushed_vertices;
	TriMesh::FaceHandle fh0 = smallMesh.face_handle(0);
	TriMesh::FaceHandle fh1 = smallMesh.face_handle(1);

	// push all vertices into the quad mesh
	for (auto v_it = smallMesh.vertices_begin(); v_it != smallMesh.vertices_end(); ++v_it) {
		TriMesh::VertexHandle vh = *v_it;
		sqmVec[vh.idx()] = smallQuadMesh.add_vertex(smallMesh.point(vh));
		already_pushed_vertices.insert(vh.idx());
	}

	vector<PolyMesh::VertexHandle> face_vhandles;
	// how many vertices of face 0 are already pushed
	int v_face_0 = 0;
	// the last pushed vertex of face 0
	int last_vert = 0;

	//go through first triangle, check every half edge
	for (auto fh_it = smallMesh.fh_iter(fh0); fh_it.is_valid(); ++fh_it) {
		TriMesh::HalfedgeHandle heh = *fh_it;
		//TriMesh::FaceHandle fhs = smallMesh.face_handle(heh);

		// push from vertex of active halfedge into vertex handle vector
		TriMesh::VertexHandle fvhs = smallMesh.from_vertex_handle(heh);
		last_vert = fvhs.idx();
		//cout << last_vert << endl;
		face_vhandles.push_back(smallMesh.vertex_handle(last_vert));
		v_face_0++;

		// check if active halfedge is connecting one, if yes, break
		TriMesh::FaceHandle fhso = smallMesh.opposite_face_handle(heh);
		if (fhso.idx() == face1) {
			break;
		}
		//TriMesh::VertexHandle fvhs = smallMesh.from_vertex_handle(heh);
		//TriMesh::VertexHandle tvhs = smallMesh.to_vertex_handle(heh);
	}

	//go through second triangle, check every half edge
	for (auto fh_it = smallMesh.fh_iter(fh1); fh_it.is_valid(); ++fh_it) {
		TriMesh::HalfedgeHandle heh = *fh_it;

		// check, whitch halfedge is active
		TriMesh::VertexHandle fvhs = smallMesh.from_vertex_handle(heh);
		int akt_from_vert = fvhs.idx();
		TriMesh::VertexHandle tvhs = smallMesh.to_vertex_handle(heh);
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
	for (auto fh_it = smallMesh.fh_iter(fh0); fh_it.is_valid(); ++fh_it) {
		TriMesh::HalfedgeHandle heh = *fh_it;
		if (v_face_0 != 0) {
			// skip all already pushed vertices
			v_face_0--;
			continue;
		}
		else {
			TriMesh::VertexHandle fvhs = smallMesh.from_vertex_handle(heh);
			face_vhandles.push_back(smallMesh.vertex_handle(fvhs.idx()));
			//cout << fvhs.idx() << endl;
		}
	}

	//cout << face_vhandles.size() << endl;
	smallQuadMesh.add_face(face_vhandles);
	//OpenMesh::IO::write_mesh(smallQuadMesh, "Created_Meshes/smallQuadMesh.off");



	/*//neighbouring faces per face
	TriMesh::HalfedgeHandle hehs = smallMesh.halfedge_handle(mesh.edge_handle(1), 0);
	TriMesh::HalfedgeHandle hehs_opp = mesh.opposite_halfedge_handle(hehs);

	TriMesh::FaceHandle fhs = smallMesh.face_handle(hehs);
	cout << "half edge 1 face: " << fhs.idx() << endl;

	TriMesh::FaceHandle fhso = smallMesh.opposite_face_handle(hehs);
	cout << "half edge 1 opposite face: " << fhso.idx() << endl; */

	/*
	// outgoing half-edges of a vertex
	TriMesh::VertexHandle vh_s = smallMesh.vertex_handle(0);
	for (auto voh_it = smallMesh.voh_iter(vh_s); voh_it.is_valid(); ++voh_it) {
	TriMesh::HalfedgeHandle heh = *voh_it;
	cout << heh.idx() << endl;
	}


	//cout << hehs.idx() << endl;
	//cout << hehs_opp.idx() << endl;
	*/

	/*
	//Mesh with hole in list
	//last vertex added with push_back
	//only the vertices are used in the mesh
	// importend for creating quad mesh
	TriMesh testMesh;
	std::vector<TriMesh::VertexHandle> tvVec(8);

	tvVec[0] = testMesh.add_vertex(TriMesh::Point(0, 0, 0));
	tvVec[1] = testMesh.add_vertex(TriMesh::Point(1, 0, 0));
	tvVec[3] = testMesh.add_vertex(TriMesh::Point(0, 1, 0));
	tvVec[4] = testMesh.add_vertex(TriMesh::Point(1, 1, 0));
	tvVec.push_back(testMesh.add_vertex(TriMesh::Point(2, 1, 0)));

	testMesh.add_face({ tvVec[0], tvVec[1], tvVec[4] });
	testMesh.add_face({ tvVec[0], tvVec[4], tvVec[3] });
	testMesh.add_face({ tvVec[1], tvVec[8], tvVec[4] });

	OpenMesh::IO::write_mesh(testMesh, "../testMesh.off");
	cout << "Test Mesh" << endl;
	for (auto v_it = testMesh.vertices_begin(); v_it != testMesh.vertices_end(); ++v_it) {
	TriMesh::VertexHandle vh = *v_it;
	cout << "vertex: " << vh.idx() << " ; coords: " << testMesh.point(vh) << endl;
	}
	int number_vert = testMesh.n_vertices();
	cout << number_vert << endl;
	TriMesh::VertexHandle t_vh = tvVec[8];// testMesh.vertex_handle(0);
	cout << testMesh.point(t_vh) << endl;
	// prints the vertex number of the point on this position in the vector
	cout << tvVec[8] << endl;
	*/

	naiv *naiv_algo = new naiv();
	blossom_quad *blossom_algo = new blossom_quad();
    CG cg(1280,720);
	naiv_algo->do_naiv_algo();
	blossom_algo->do_blossom_algo();
    cg.startMainLoop();
    return 0;

}
