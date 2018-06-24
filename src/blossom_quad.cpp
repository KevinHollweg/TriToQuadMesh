#include "blossom_quad.h"

using namespace std;
using namespace glm;


blossom_quad::blossom_quad() {}

blossom_quad::~blossom_quad() {}

PolyMesh workMesh;
std::vector<PolyMesh::VertexHandle> workMesh_Vertex_Vec(15);
PolyMesh blossomMesh_step1;
std::vector<PolyMesh::VertexHandle> blossomMesh_step1_vertex_vec;

vector<PolyMesh::FaceHandle> faces_with_one_graph_edge;

bool debug = false;


void blossom_quad::do_blossom_algo() {
	// create work mesh, will later be changed
	create_test_mesh();

	// property for edge in connection graph
	OpenMesh::HPropHandleT<int>		hprop_graph_edge_idx;
	workMesh.add_property(hprop_graph_edge_idx, "hprop_graph_edge_idx");
	// property, if mesh edge has alredy edge in connection graph
	OpenMesh::HPropHandleT<bool>		hprop_has_graph_edge;
	workMesh.add_property(hprop_has_graph_edge, "hprop_has_graph_edge");
	// property, if face was already used for quad generation
	OpenMesh::FPropHandleT<int>	fprop_graph_edge_count;
	workMesh.add_property(fprop_graph_edge_count, "fprop_graph_edge_count");
	// property, if face has borders
	OpenMesh::FPropHandleT<int>	fprop_borders_count;
	workMesh.add_property(fprop_borders_count, "fprop_borders_count");
	// property, if a vertex was already used for an external edge
	OpenMesh::VPropHandleT<bool>	vprop_has_external_edge;
	workMesh.add_property(vprop_has_external_edge, "vprop_has_external_edge");
	// property for external edge in connection graph
	OpenMesh::VPropHandleT<int>		vprop_external_edge_idx;
	workMesh.add_property(vprop_external_edge_idx, "vprop_external_edge_idx");

	// connectivity graph 
	graph *connection_graph = new graph();
	// create connectivity graph
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
				sort_edge *m_sorted_edge = new sort_edge(connection_graph->edges_counter, internal_edge->cost);
				connection_graph->sorted_edges.push_back(*m_sorted_edge);
				connection_graph->edges_counter++;
				workMesh.property(hprop_has_graph_edge, heh) = true;
				workMesh.property(hprop_has_graph_edge, oheh) = true;
				face_graph_edge_count++;				
			}
			else if(fhso.idx() == -1 ) {
				border_count++;
			}
			else if (fhso.idx() != -1 && workMesh.property(hprop_has_graph_edge, heh) == true) {
				if (!connection_graph->edges[workMesh.property(hprop_graph_edge_idx, heh)].bad) {
					face_graph_edge_count++;
				}
			}
		}
		// add external edges
		// if face has one border
		if (border_count == 1) {
			for (auto fh_it = workMesh.fh_iter(fh); fh_it.is_valid(); ++fh_it) {
				PolyMesh::HalfedgeHandle heh = *fh_it;
				PolyMesh::FaceHandle fhs = workMesh.face_handle(heh);
				PolyMesh::FaceHandle fhso = workMesh.opposite_face_handle(heh);
				// find halfedge, that is a borader edge
				if (fhso.idx() == -1) {
					// edge containing the "from vertex" of the half edge
					PolyMesh::VertexHandle fromVertH = workMesh.from_vertex_handle(heh);
					if (!workMesh.property(vprop_has_external_edge, fromVertH)) {						
						graph_edge *external_edge = new graph_edge();
						external_edge->face0 = fhs;
						external_edge->connecting_vertex = fromVertH;
						external_edge->cost = EXTERNAL_EDGE_COST;
						int count_incomming_hes = 0;
						// finde connected edge face
						for (auto vheh_it = workMesh.voh_iter(fromVertH); vheh_it.is_valid(); ++vheh_it) {
							count_incomming_hes++;
							PolyMesh::HalfedgeHandle vheh = *vheh_it;
							PolyMesh::FaceHandle external_face = workMesh.face_handle(vheh);
							PolyMesh::FaceHandle conencted_face = workMesh.opposite_face_handle(vheh);
							if (external_face.idx() == -1) {
								external_edge->face1 = conencted_face;
							}
						}
						// faces are neighbours
						if (count_incomming_hes == 3) {
							external_edge->~graph_edge();
						}
						// faces are seperated by one other face
						else if (count_incomming_hes == 4) {
							workMesh.property(vprop_has_external_edge, fromVertH) = true;
							workMesh.property(vprop_external_edge_idx, fromVertH) = connection_graph->external_edges_counter;
							connection_graph->external_edges_counter++;
							face_graph_edge_count++;
							external_edge->use_edge_swap = true;
							connection_graph->external_edges.push_back(*external_edge);
						}
						// faces are seperated by more than one face
						else if (count_incomming_hes >= 5) {
							workMesh.property(vprop_has_external_edge, fromVertH) = true;
							workMesh.property(vprop_external_edge_idx, fromVertH) = connection_graph->external_edges_counter;
							connection_graph->external_edges_counter++;
							face_graph_edge_count++;
							external_edge->use_vertex_duplication = true;
							connection_graph->external_edges.push_back(*external_edge);
						}
					}
					else {
						face_graph_edge_count++;
					}
					// edge containing the "to vertex" of the half edge
					PolyMesh::VertexHandle toVertH = workMesh.to_vertex_handle(heh);
					if (!workMesh.property(vprop_has_external_edge, toVertH)) {
						graph_edge *external_edge = new graph_edge();
						external_edge->face0 = fhs;
						external_edge->connecting_vertex = toVertH;
						external_edge->cost = EXTERNAL_EDGE_COST;
						int count_outgoing_hes = 0;
						// finde connected edge face
						for (auto vheh_it = workMesh.vih_iter(toVertH); vheh_it.is_valid(); ++vheh_it) {
							count_outgoing_hes++;
							PolyMesh::HalfedgeHandle vheh = *vheh_it;
							PolyMesh::FaceHandle external_face = workMesh.face_handle(vheh);
							PolyMesh::FaceHandle conencted_face = workMesh.opposite_face_handle(vheh);
							if (external_face.idx() == -1) {
								external_edge->face1 = conencted_face;
							}
						}
						// faces are neighbours
						if (count_outgoing_hes == 3) {
							external_edge->~graph_edge();
						}
						// faces are seperated by one other face
						else if (count_outgoing_hes == 4) {
							workMesh.property(vprop_has_external_edge, toVertH) = true;
							workMesh.property(vprop_external_edge_idx, toVertH) = connection_graph->external_edges_counter;
							connection_graph->external_edges_counter++;
							face_graph_edge_count++;
							external_edge->use_edge_swap = true;
							connection_graph->external_edges.push_back(*external_edge);
						}
						// faces are seperated by more than one face
						else if (count_outgoing_hes >= 5) {
							workMesh.property(vprop_has_external_edge, toVertH) = true;
							workMesh.property(vprop_external_edge_idx, toVertH) = connection_graph->external_edges_counter;
							connection_graph->external_edges_counter++;
							face_graph_edge_count++;
							external_edge->use_vertex_duplication = true;
							connection_graph->external_edges.push_back(*external_edge);
						}
					}
					else {
						face_graph_edge_count++;
					}
				}
			}
		}
		// if face has two borders
		else if (border_count == 2) {
			for (auto fh_it = workMesh.fh_iter(fh); fh_it.is_valid(); ++fh_it) {
				PolyMesh::HalfedgeHandle heh = *fh_it;
				PolyMesh::FaceHandle fhs = workMesh.face_handle(heh);
				PolyMesh::FaceHandle fhso = workMesh.opposite_face_handle(heh);
				// find halfedge, that isn`t a borader edge
				if (fhso.idx() != -1) {
					// edge containing the "from vertex" of the half edge
					PolyMesh::VertexHandle fromVertH = workMesh.from_vertex_handle(heh);
					if (!workMesh.property(vprop_has_external_edge, fromVertH)) {						
						graph_edge *external_edge = new graph_edge();
						external_edge->face0 = fhs;
						external_edge->connecting_vertex = fromVertH;
						external_edge->cost = EXTERNAL_EDGE_COST;						
						int count_incomming_hes = 0;
						// finde connected edge face
						for (auto vheh_it = workMesh.vih_iter(fromVertH); vheh_it.is_valid(); ++vheh_it) {
							count_incomming_hes++;
							PolyMesh::HalfedgeHandle vheh = *vheh_it;
							PolyMesh::FaceHandle external_face = workMesh.face_handle(vheh);
							PolyMesh::FaceHandle conencted_face = workMesh.opposite_face_handle(vheh);
							if (external_face.idx() == -1) {
								external_edge->face1 = conencted_face;
							}
						}
						// faces are neighbours
						if (count_incomming_hes == 3) {
							external_edge->~graph_edge();
						}
						// faces are seperated by one other face
						else if (count_incomming_hes == 4) {
							workMesh.property(vprop_has_external_edge, fromVertH) = true;
							workMesh.property(vprop_external_edge_idx, fromVertH) = connection_graph->external_edges_counter;
							connection_graph->external_edges_counter++;
							face_graph_edge_count++;
							external_edge->use_edge_swap = true;
							connection_graph->external_edges.push_back(*external_edge);
						}
						// faces are seperated by more than one face
						else if (count_incomming_hes >= 5) {
							workMesh.property(vprop_has_external_edge, fromVertH) = true;
							workMesh.property(vprop_external_edge_idx, fromVertH) = connection_graph->external_edges_counter;
							connection_graph->external_edges_counter++;
							face_graph_edge_count++;
							external_edge->use_vertex_duplication =  true;
							connection_graph->external_edges.push_back(*external_edge);
						}
					}
					else {
						face_graph_edge_count++;
					}
					// edge containing the "to vertex" of the half edge
					PolyMesh::VertexHandle toVertH = workMesh.to_vertex_handle(heh);
					if (!workMesh.property(vprop_has_external_edge, toVertH)) {
						graph_edge *external_edge = new graph_edge();
						external_edge->face0 = fhs;
						external_edge->connecting_vertex = toVertH;
						external_edge->cost = EXTERNAL_EDGE_COST;
						int count_outgoing_hes = 0;
						// finde connected edge face
						for (auto vheh_it = workMesh.voh_iter(toVertH); vheh_it.is_valid(); ++vheh_it) {
							count_outgoing_hes++;
							PolyMesh::HalfedgeHandle vheh = *vheh_it;
							PolyMesh::FaceHandle external_face = workMesh.face_handle(vheh);
							PolyMesh::FaceHandle conencted_face = workMesh.opposite_face_handle(vheh);
							if (external_face.idx() == -1) {
								external_edge->face1 = conencted_face;
							}
						}
						// faces are neighbours
						if (count_outgoing_hes == 3) {
							external_edge->~graph_edge();
						}
						// faces are seperated by one other face
						else if (count_outgoing_hes == 4) {
							workMesh.property(vprop_has_external_edge, toVertH) = true;
							workMesh.property(vprop_external_edge_idx, toVertH) = connection_graph->external_edges_counter;
							connection_graph->external_edges_counter++;
							face_graph_edge_count++;
							external_edge->use_edge_swap = true;
							connection_graph->external_edges.push_back(*external_edge);
						}
						// faces are seperated by more than one face
						else if (count_outgoing_hes >= 5) {
							workMesh.property(vprop_has_external_edge, toVertH) = true;
							workMesh.property(vprop_external_edge_idx, toVertH) = connection_graph->external_edges_counter;
							connection_graph->external_edges_counter++;
							face_graph_edge_count++;
							external_edge->use_vertex_duplication = true;
							connection_graph->external_edges.push_back(*external_edge);
						}
					}
					else {
						face_graph_edge_count++;
					}
				}
			}
		}
		else if(border_count == 3) {
			cout << "Error" << endl;
			cout << "Face " << fh.idx() << endl;
		}
		workMesh.property(fprop_borders_count, fh) = border_count;
	
		workMesh.property(fprop_graph_edge_count, fh) = face_graph_edge_count;
		//cout << "Face " << fh.idx() << " , edge_count " << face_graph_edge_count << endl;
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
		if (workMesh.property(fprop_graph_edge_count, fh) == 1) {
			faces_with_one_graph_edge.push_back(fh);
		}
	}

	// create step one result mesh
	// copy all points
	int point_count = 0;
	for (auto v_it = workMesh.vertices_begin(); v_it != workMesh.vertices_end(); ++v_it) {
		PolyMesh::VertexHandle vh = *v_it;
		blossomMesh_step1_vertex_vec.push_back(blossomMesh_step1.add_vertex(workMesh.point(vh)));
		point_count++;
	}
	sort(connection_graph->sorted_edges.begin(), connection_graph->sorted_edges.end());
	//connection_graph->print_sorted_edges();

	// connect internal edges
	// TODO comments!!!!
	int internal_edges_it = 0;
	while (true) {
		if (faces_with_one_graph_edge.size() != 0) {
			PolyMesh::FaceHandle act_face = faces_with_one_graph_edge[0];
			faces_with_one_graph_edge.erase(faces_with_one_graph_edge.begin());
			for (auto fh_it = workMesh.fh_iter(act_face); fh_it.is_valid(); ++fh_it) {
				PolyMesh::HalfedgeHandle heh = *fh_it;
				if (!connection_graph->edges[workMesh.property(hprop_graph_edge_idx, heh)].bad && !connection_graph->edges[workMesh.property(hprop_graph_edge_idx, heh)].no_longer_available) {
					blossomMesh_step1.add_face(connection_graph->edges[workMesh.property(hprop_graph_edge_idx, heh)].quad_vertices);
					PolyMesh::FaceHandle face0 = connection_graph->edges[workMesh.property(hprop_graph_edge_idx, heh)].face0;
					PolyMesh::FaceHandle face1 = connection_graph->edges[workMesh.property(hprop_graph_edge_idx, heh)].face1;
					workMesh.property(fprop_graph_edge_count, face0)--;
					workMesh.property(fprop_graph_edge_count, face1)--;
					if (face0.idx() != act_face.idx()) {
						for (auto other_fh_it = workMesh.fh_iter(face0); other_fh_it.is_valid(); ++other_fh_it) {
							PolyMesh::HalfedgeHandle face0_heh = *other_fh_it;
							if (!connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face0_heh)].bad && workMesh.property(hprop_has_graph_edge, face0_heh)) {
								connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face0_heh)].no_longer_available = true;
								PolyMesh::FaceHandle other_face0 = connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face0_heh)].face0;
								PolyMesh::FaceHandle other_face1 = connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face0_heh)].face1;
								if (other_face0.idx() != face0.idx()) {
									workMesh.property(fprop_graph_edge_count, other_face0)--;
									if (workMesh.property(fprop_graph_edge_count, other_face0) == 1) {
										faces_with_one_graph_edge.push_back(other_face0);
									}
								}
								else {
									workMesh.property(fprop_graph_edge_count, other_face1)--;
									if (workMesh.property(fprop_graph_edge_count, other_face1) == 1) {
										faces_with_one_graph_edge.push_back(other_face1);
									}
								}
							}	
						}
						if (workMesh.property(fprop_borders_count, face0) != 0) {
							for (auto fv_it = workMesh.fv_iter(face0); fv_it.is_valid(); ++fv_it) {
								PolyMesh::VertexHandle face0_vit = *fv_it;
								if (workMesh.property(vprop_has_external_edge, face0_vit)) {
									connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, face0_vit)].no_longer_available = true;
									PolyMesh::FaceHandle other_face0 = connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, face0_vit)].face0;
									PolyMesh::FaceHandle other_face1 = connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, face0_vit)].face1;
									if (other_face0.idx() != face0.idx()) {
										workMesh.property(fprop_graph_edge_count, other_face0)--;
										if (workMesh.property(fprop_graph_edge_count, other_face0) == 1) {
											faces_with_one_graph_edge.push_back(other_face0);
										}
									}
									else {
										workMesh.property(fprop_graph_edge_count, other_face1)--;
										if (workMesh.property(fprop_graph_edge_count, other_face1) == 1) {
											faces_with_one_graph_edge.push_back(other_face1);
										}
									}
								}
							}
						}	
					}
					else {
						for (auto other_fh_it = workMesh.fh_iter(face1); other_fh_it.is_valid(); ++other_fh_it) {
							PolyMesh::HalfedgeHandle face1_heh = *other_fh_it;
							if (!connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face1_heh)].bad && workMesh.property(hprop_has_graph_edge, face1_heh)) {
								connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face1_heh)].no_longer_available = true;
								PolyMesh::FaceHandle other_face0 = connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face1_heh)].face0;
								PolyMesh::FaceHandle other_face1 = connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face1_heh)].face1;
								if (other_face0.idx() != face1.idx()) {
									workMesh.property(fprop_graph_edge_count, other_face0)--;
									if (workMesh.property(fprop_graph_edge_count, other_face0) == 1) {
										faces_with_one_graph_edge.push_back(other_face0);
									}
								}
								else {
									workMesh.property(fprop_graph_edge_count, other_face1)--;
									if (workMesh.property(fprop_graph_edge_count, other_face1) == 1) {
										faces_with_one_graph_edge.push_back(other_face1);
									}
								}
							}	
						}
						if (workMesh.property(fprop_borders_count, face1) != 0) {
							for (auto fv_it = workMesh.fv_iter(face1); fv_it.is_valid(); ++fv_it) {
								PolyMesh::VertexHandle face1_vit = *fv_it;
								if (workMesh.property(vprop_has_external_edge, face1_vit)) {
									connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, face1_vit)].no_longer_available = true;
									PolyMesh::FaceHandle other_face0 = connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, face1_vit)].face0;
									PolyMesh::FaceHandle other_face1 = connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, face1_vit)].face1;
									if (other_face0.idx() != face1.idx()) {
										workMesh.property(fprop_graph_edge_count, other_face0)--;
										if (workMesh.property(fprop_graph_edge_count, other_face0) == 1) {
											faces_with_one_graph_edge.push_back(other_face0);
										}
									}
									else {
										workMesh.property(fprop_graph_edge_count, other_face1)--;
										if (workMesh.property(fprop_graph_edge_count, other_face1) == 1) {
											faces_with_one_graph_edge.push_back(other_face1);
										}
									}
								}
							}
						}
					}
				}
			}
			
			for (auto fv_it = workMesh.fv_iter(act_face); fv_it.is_valid(); ++fv_it) {
				PolyMesh::VertexHandle vh = *fv_it;
				if ( !connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, vh)].no_longer_available && (workMesh.property(fprop_borders_count, act_face) != 0) ) {
					connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, vh)].to_be_used = true;
					PolyMesh::FaceHandle face0 = connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, vh)].face0;
					PolyMesh::FaceHandle face1 = connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, vh)].face1;
					workMesh.property(fprop_graph_edge_count, face0)--;
					workMesh.property(fprop_graph_edge_count, face1)--;
					if (face0.idx() != act_face.idx()) {
						for (auto other_fh_it = workMesh.fh_iter(face0); other_fh_it.is_valid(); ++other_fh_it) {
							PolyMesh::HalfedgeHandle face0_heh = *other_fh_it;
							if (!connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face0_heh)].bad && workMesh.property(hprop_has_graph_edge, face0_heh)) {
								connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face0_heh)].no_longer_available = true;
								PolyMesh::FaceHandle other_face0 = connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face0_heh)].face0;
								PolyMesh::FaceHandle other_face1 = connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face0_heh)].face1;
								if (other_face0.idx() != face0.idx()) {
									workMesh.property(fprop_graph_edge_count, other_face0)--;
									if (workMesh.property(fprop_graph_edge_count, other_face0) == 1) {
										faces_with_one_graph_edge.push_back(other_face0);
									}
								}
								else {
									workMesh.property(fprop_graph_edge_count, other_face1)--;
									if (workMesh.property(fprop_graph_edge_count, other_face1) == 1) {
										faces_with_one_graph_edge.push_back(other_face1);
									}
								}
							}
						}
						if (workMesh.property(fprop_borders_count, face0) != 0) {
							for (auto fv_it = workMesh.fv_iter(face0); fv_it.is_valid(); ++fv_it) {
								PolyMesh::VertexHandle face0_vit = *fv_it;
								if (workMesh.property(vprop_has_external_edge, face0_vit)) {
									connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, face0_vit)].no_longer_available = true;
									PolyMesh::FaceHandle other_face0 = connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, face0_vit)].face0;
									PolyMesh::FaceHandle other_face1 = connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, face0_vit)].face1;
									if (other_face0.idx() != face0.idx()) {
										workMesh.property(fprop_graph_edge_count, other_face0)--;
										if (workMesh.property(fprop_graph_edge_count, other_face0) == 1) {
											faces_with_one_graph_edge.push_back(other_face0);
										}
									}
									else {
										workMesh.property(fprop_graph_edge_count, other_face1)--;
										if (workMesh.property(fprop_graph_edge_count, other_face1) == 1) {
											faces_with_one_graph_edge.push_back(other_face1);
										}
									}
								}	
							}
						}	
					}
					else {
						for (auto other_fh_it = workMesh.fh_iter(face1); other_fh_it.is_valid(); ++other_fh_it) {
							PolyMesh::HalfedgeHandle face1_heh = *other_fh_it;
							if (!connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face1_heh)].bad && workMesh.property(hprop_has_graph_edge, face1_heh)) {
								connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face1_heh)].no_longer_available = true;
								PolyMesh::FaceHandle other_face0 = connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face1_heh)].face0;
								PolyMesh::FaceHandle other_face1 = connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face1_heh)].face1;
								if (other_face0.idx() != face1.idx()) {
									workMesh.property(fprop_graph_edge_count, other_face0)--;
									if (workMesh.property(fprop_graph_edge_count, other_face0) == 1) {
										faces_with_one_graph_edge.push_back(other_face0);
									}
								}
								else {
									workMesh.property(fprop_graph_edge_count, other_face1)--;
									if (workMesh.property(fprop_graph_edge_count, other_face1) == 1) {
										faces_with_one_graph_edge.push_back(other_face1);
									}
								}
							}
						}
						if (workMesh.property(fprop_borders_count, face1) != 0) {
							for (auto fv_it = workMesh.fv_iter(face1); fv_it.is_valid(); ++fv_it) {
								PolyMesh::VertexHandle face1_vit = *fv_it;
								if (workMesh.property(vprop_has_external_edge, face1_vit)) {
									connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, face1_vit)].no_longer_available = true;
									PolyMesh::FaceHandle other_face0 = connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, face1_vit)].face0;
									PolyMesh::FaceHandle other_face1 = connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, face1_vit)].face1;
									if (other_face0.idx() != face1.idx()) {
										workMesh.property(fprop_graph_edge_count, other_face0)--;
										if (workMesh.property(fprop_graph_edge_count, other_face0) == 1) {
											faces_with_one_graph_edge.push_back(other_face0);
										}
									}
									else {
										workMesh.property(fprop_graph_edge_count, other_face1)--;
										if (workMesh.property(fprop_graph_edge_count, other_face1) == 1) {
											faces_with_one_graph_edge.push_back(other_face1);
										}
									}
								}
							}
						}	
					}
				}
			}
		}
		else {
			graph_edge act_edge = connection_graph->edges[connection_graph->sorted_edges[internal_edges_it].m_orig_position];
			if (!act_edge.no_longer_available && !act_edge.bad) {
				blossomMesh_step1.add_face(act_edge.quad_vertices);
				PolyMesh::FaceHandle face0 = act_edge.face0;
				PolyMesh::FaceHandle face1 = act_edge.face1;
				workMesh.property(fprop_graph_edge_count, face0)--;
				workMesh.property(fprop_graph_edge_count, face1)--;
				
				for (auto other_fh_it = workMesh.fh_iter(face0); other_fh_it.is_valid(); ++other_fh_it) {
					PolyMesh::HalfedgeHandle face0_heh = *other_fh_it;
					if (!connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face0_heh)].bad) {
						connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face0_heh)].no_longer_available = true;
						PolyMesh::FaceHandle other_face0 = connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face0_heh)].face0;
						PolyMesh::FaceHandle other_face1 = connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face0_heh)].face1;
						if (other_face0.idx() != face0.idx()) {
							workMesh.property(fprop_graph_edge_count, other_face0)--;
							if (workMesh.property(fprop_graph_edge_count, other_face0) == 1) {
								faces_with_one_graph_edge.push_back(other_face0);
							}
						}
						else {
							workMesh.property(fprop_graph_edge_count, other_face1)--;
							if (workMesh.property(fprop_graph_edge_count, other_face1) == 1) {
								faces_with_one_graph_edge.push_back(other_face1);
							}
						}
					}
				}
				if (workMesh.property(fprop_borders_count, face0) != 0) {
					for (auto fv_it = workMesh.fv_iter(face0); fv_it.is_valid(); ++fv_it) {
						PolyMesh::VertexHandle face0_vit = *fv_it;
						connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, face0_vit)].no_longer_available = true;
						PolyMesh::FaceHandle other_face0 = connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, face0_vit)].face0;
						PolyMesh::FaceHandle other_face1 = connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, face0_vit)].face1;
						if (other_face0.idx() != face0.idx()) {
							workMesh.property(fprop_graph_edge_count, other_face0)--;
							if (workMesh.property(fprop_graph_edge_count, other_face0) == 1) {
								faces_with_one_graph_edge.push_back(other_face0);
							}
						}
						else {
							workMesh.property(fprop_graph_edge_count, other_face1)--;
							if (workMesh.property(fprop_graph_edge_count, other_face1) == 1) {
								faces_with_one_graph_edge.push_back(other_face1);
							}
						}
					}
				}
				
				for (auto other_fh_it = workMesh.fh_iter(face1); other_fh_it.is_valid(); ++other_fh_it) {
					PolyMesh::HalfedgeHandle face1_heh = *other_fh_it;
					if (!connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face1_heh)].bad) {
						connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face1_heh)].no_longer_available = true;
						PolyMesh::FaceHandle other_face0 = connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face1_heh)].face0;
						PolyMesh::FaceHandle other_face1 = connection_graph->edges[workMesh.property(hprop_graph_edge_idx, face1_heh)].face1;
						if (other_face0.idx() != face1.idx()) {
							workMesh.property(fprop_graph_edge_count, other_face0)--;
							if (workMesh.property(fprop_graph_edge_count, other_face0) == 1) {
								faces_with_one_graph_edge.push_back(other_face0);
							}
						}
						else {
							workMesh.property(fprop_graph_edge_count, other_face1)--;
							if (workMesh.property(fprop_graph_edge_count, other_face1) == 1) {
								faces_with_one_graph_edge.push_back(other_face1);
							}
						}
					}
				}
				if (workMesh.property(fprop_borders_count, face1) != 0) {
					for (auto fv_it = workMesh.fv_iter(face1); fv_it.is_valid(); ++fv_it) {
						PolyMesh::VertexHandle face1_vit = *fv_it;
						connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, face1_vit)].no_longer_available = true;
						PolyMesh::FaceHandle other_face0 = connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, face1_vit)].face0;
						PolyMesh::FaceHandle other_face1 = connection_graph->external_edges[workMesh.property(vprop_external_edge_idx, face1_vit)].face1;
						if (other_face0.idx() != face1.idx()) {
							workMesh.property(fprop_graph_edge_count, other_face0)--;
							if (workMesh.property(fprop_graph_edge_count, other_face0) == 1) {
								faces_with_one_graph_edge.push_back(other_face0);
							}
						}
						else {
							workMesh.property(fprop_graph_edge_count, other_face1)--;
							if (workMesh.property(fprop_graph_edge_count, other_face1) == 1) {
								faces_with_one_graph_edge.push_back(other_face1);
							}
						}
					}
				}				
			}
			internal_edges_it++;
		}
		if (faces_with_one_graph_edge.size() == 0 && internal_edges_it == connection_graph->edges_counter) {
			break;
		}
	}


	// connect external edges with vertex duplication
	for (int i = 0; i < connection_graph->external_edges_counter; i++) {
		if (connection_graph->external_edges[i].use_vertex_duplication && connection_graph->external_edges[i].to_be_used ) { 
			PolyMesh::VertexHandle con_vert = connection_graph->external_edges[i].connecting_vertex;
			PolyMesh::FaceHandle face0 = connection_graph->external_edges[i].face0;
			PolyMesh::FaceHandle face1 = connection_graph->external_edges[i].face1;

			vec3 *y_vert = new vec3(workMesh.point(con_vert)[0], workMesh.point(con_vert)[1], workMesh.point(con_vert)[2]);
			vec3 *x_0_vert;
			vec3 *x_1_vert;
			int from_vertex_face = 0;
			int to_vertex_face = 0;

			for (auto vheh_it = workMesh.vih_iter(con_vert); vheh_it.is_valid(); ++vheh_it) {
				PolyMesh::HalfedgeHandle heh = *vheh_it;
				PolyMesh::HalfedgeHandle oheh = workMesh.opposite_halfedge_handle(heh);
				PolyMesh::FaceHandle fhs = workMesh.face_handle(heh);
				PolyMesh::FaceHandle ofhs = workMesh.face_handle(oheh);
				if (fhs.idx() == -1 ) {
					x_0_vert = new vec3(workMesh.point(workMesh.from_vertex_handle(heh))[0], workMesh.point(workMesh.from_vertex_handle(heh))[1], workMesh.point(workMesh.from_vertex_handle(heh))[2]);
					from_vertex_face = ofhs.idx();
				}
				else if (ofhs.idx() == -1) {
					x_1_vert = new vec3(workMesh.point(workMesh.to_vertex_handle(oheh))[0], workMesh.point(workMesh.to_vertex_handle(oheh))[1], workMesh.point(workMesh.to_vertex_handle(oheh))[2]);
					to_vertex_face = fhs.idx();
				}
			}
			vec3 edge_x1_x0 = *x_1_vert - *x_0_vert;
			vec3 *mid_x1_x0 = new vec3(x_0_vert->x + 0.5 * edge_x1_x0.x, x_0_vert->y + 0.5 * edge_x1_x0.y, x_0_vert->z + 0.5 * edge_x1_x0.z);
			vec3 edge_mid_x1_x0_y = *mid_x1_x0 - *y_vert;
			vec3 *dup_vert = new vec3(y_vert->x + 0.25 * edge_mid_x1_x0_y.x, y_vert->y + 0.25 * edge_mid_x1_x0_y.y, y_vert->z + 0.25 * edge_mid_x1_x0_y.z);
			blossomMesh_step1_vertex_vec.push_back(blossomMesh_step1.add_vertex(PolyMesh::Point(dup_vert->x, dup_vert->y, dup_vert->z)));
			int dup_vert_idx = point_count;
			point_count++;
			if (face0.idx() == from_vertex_face) {
				// face0 from -> new vert after connection vertex
				vector<PolyMesh::VertexHandle> quad_face0;
				for (auto face0_vert_it = workMesh.fv_iter(face0); face0_vert_it.is_valid(); ++face0_vert_it) {
					PolyMesh::VertexHandle vh_face0 = *face0_vert_it;
					if (vh_face0.idx() == con_vert.idx()) {
						quad_face0.push_back(blossomMesh_step1_vertex_vec[vh_face0.idx()]);
						quad_face0.push_back(blossomMesh_step1_vertex_vec[dup_vert_idx]);						
					}
					else {
						quad_face0.push_back(blossomMesh_step1_vertex_vec[vh_face0.idx()]);
					}
				}
				blossomMesh_step1.add_face(quad_face0);
				// face1 to -> new vert before conenction vertex
				vector<PolyMesh::VertexHandle> quad_face1;
				for (auto face1_vert_it = workMesh.fv_iter(face1); face1_vert_it.is_valid(); ++face1_vert_it) {
					PolyMesh::VertexHandle vh_face1 = *face1_vert_it;
					if (vh_face1.idx() == con_vert.idx()) {
						quad_face1.push_back(blossomMesh_step1_vertex_vec[dup_vert_idx]);
						quad_face1.push_back(blossomMesh_step1_vertex_vec[vh_face1.idx()]);						
					}
					else {
						quad_face1.push_back(blossomMesh_step1_vertex_vec[vh_face1.idx()]);
					}
				}
				blossomMesh_step1.add_face(quad_face1);
			}
			else {
				// face0 to -> new vert before conenction vertex
				vector<PolyMesh::VertexHandle> quad_face0;
				for (auto face0_vert_it = workMesh.fv_iter(face0); face0_vert_it.is_valid(); ++face0_vert_it) {
					PolyMesh::VertexHandle vh_face0 = *face0_vert_it;
					//cout << vh_face0.idx() << endl;
					if (vh_face0.idx() == con_vert.idx()) {						
						quad_face0.push_back(blossomMesh_step1_vertex_vec[dup_vert_idx]);	
						quad_face0.push_back(blossomMesh_step1_vertex_vec[vh_face0.idx()]);
					}
					else {
						quad_face0.push_back(blossomMesh_step1_vertex_vec[vh_face0.idx()]);
					}
				}
				blossomMesh_step1.add_face(quad_face0);
				// face1 from -> new vert after connection vertex 
				vector<PolyMesh::VertexHandle> quad_face1;
				for (auto face1_vert_it = workMesh.fv_iter(face1); face1_vert_it.is_valid(); ++face1_vert_it) {
					PolyMesh::VertexHandle vh_face1 = *face1_vert_it;
					if (vh_face1.idx() == con_vert.idx()) {												
						quad_face1.push_back(blossomMesh_step1_vertex_vec[vh_face1.idx()]);
						quad_face1.push_back(blossomMesh_step1_vertex_vec[dup_vert_idx]);
					}
					else {
						quad_face1.push_back(blossomMesh_step1_vertex_vec[vh_face1.idx()]);
					}
				}
				blossomMesh_step1.add_face(quad_face1);
			}
		}
	}

	OpenMesh::IO::write_mesh(blossomMesh_step1, "Created_Meshes/blossomMesh_step1.off");

	// conenct extenal edges with edge swap
	for (int i = 0; i < connection_graph->external_edges_counter; i++) {	
		if (connection_graph->external_edges[i].use_edge_swap && connection_graph->external_edges[i].to_be_used) {
			PolyMesh::VertexHandle con_vert = connection_graph->external_edges[i].connecting_vertex;
			PolyMesh::FaceHandle old_face1 = connection_graph->external_edges[i].face0;
			PolyMesh::FaceHandle old_face2 = connection_graph->external_edges[i].face1;
			PolyMesh::FaceHandle new_face;
			PolyMesh::VertexHandle old_vert_1;
			PolyMesh::VertexHandle old_vert_2;
			PolyMesh::VertexHandle quad_vert_1;
			PolyMesh::VertexHandle quad_vert_2;
			PolyMesh::VertexHandle quad_corner;

			// get face from new mesh, and get vertex handles from new mesh
			int edge_index = 0;
			for (auto voh_it_blossom = blossomMesh_step1.voh_iter(con_vert); voh_it_blossom.is_valid(); ++voh_it_blossom) {
				edge_index++;
				PolyMesh::HalfedgeHandle heh_new = *voh_it_blossom;
				if (edge_index == 1) {
					quad_vert_1 = blossomMesh_step1.to_vertex_handle(heh_new);
					new_face = blossomMesh_step1.face_handle(heh_new);
				}
				else if (edge_index == 2) {
					quad_vert_2 = blossomMesh_step1.to_vertex_handle(heh_new);
				}
				else {
					cout << "Error in edge swap" << endl;
				}
			}
			// get vertex handles from old mesh
			for (auto voh_it_work = workMesh.voh_iter(con_vert); voh_it_work.is_valid(); ++voh_it_work) {
				PolyMesh::HalfedgeHandle heh_old = *voh_it_work;
				PolyMesh::FaceHandle fh = workMesh.face_handle(heh_old);
				PolyMesh::FaceHandle opposit_fh = workMesh.opposite_face_handle(heh_old);
				if (fh.idx() == -1) {
					old_vert_2 = workMesh.to_vertex_handle(heh_old);
				}
				else if (opposit_fh.idx() == -1) {
					old_vert_1 = workMesh.to_vertex_handle(heh_old);
				}
			}
			// get quad corner opposit to connecting vertex
			for (auto fh_it = blossomMesh_step1.fh_iter(new_face); fh_it.is_valid(); fh_it++) {
				PolyMesh::HalfedgeHandle heh_new = *fh_it;
				PolyMesh::VertexHandle vh_new = blossomMesh_step1.from_vertex_handle(heh_new);
				if ( (vh_new.idx() != con_vert.idx()) && (vh_new.idx() != quad_vert_1.idx()) && (vh_new.idx() != quad_vert_2.idx()) ) {
					quad_corner = vh_new;
				}
			}

			// delete old face
			blossomMesh_step1.request_face_status();
			blossomMesh_step1.delete_face(new_face, false);
			blossomMesh_step1.garbage_collection();

			// add new faces
			blossomMesh_step1.request_face_status();
			blossomMesh_step1.add_face({ old_vert_1, quad_vert_1, quad_corner, con_vert });
			blossomMesh_step1.request_face_status();
			blossomMesh_step1.add_face({ con_vert, quad_corner, quad_vert_2, old_vert_2 });
		}
	}

	OpenMesh::IO::write_mesh(blossomMesh_step1, "Created_Meshes/blossomMesh_step2.off");

	// vertex smoothing

	OpenMesh::IO::write_mesh(blossomMesh_step1, "Created_Meshes/blossomMesh_step3.off");

	// remove unnecessary vertices 
	remove_unnecessary_vertices();

	OpenMesh::IO::write_mesh(blossomMesh_step1, "Created_Meshes/blossomMesh_step4.off");

	// remove unnecessary faces
	point_count = remove_unnecessary_faces(point_count);

	OpenMesh::IO::write_mesh(blossomMesh_step1, "Created_Meshes/blossomMesh_step5.off");

	// connection_graph->print_graph();


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

	set<int> already_pushed_vertices;
	PolyMesh::FaceHandle fh0 = workMesh.face_handle(face0);
	PolyMesh::FaceHandle fh1 = workMesh.face_handle(face1);

	vector<PolyMesh::VertexHandle> face_vhandles;
	// how many vertices of face 0 are already pushed
	int v_face_0 = 0;
	// the last pushed vertex of face 0
	int last_vert = 0;

	//go through first triangle, check every half edge
	for (auto fh_it = workMesh.fh_iter(fh0); fh_it.is_valid(); ++fh_it) {
		PolyMesh::HalfedgeHandle heh_t1 = *fh_it;
		//TriMesh::FaceHandle fhs = smallMesh.face_handle(heh);

		// push from vertex of active halfedge into vertex handle vector
		PolyMesh::VertexHandle fvhs = workMesh.from_vertex_handle(heh_t1);
		last_vert = fvhs.idx();

		face_vhandles.push_back(workMesh.vertex_handle(last_vert));
		v_face_0++;

		// check if active halfedge is connecting one, if yes, break
		PolyMesh::FaceHandle fhso = workMesh.opposite_face_handle(heh_t1);
		//cout << fhso.idx() << endl;
		if (fhso.idx() == face1) {
			break;
		}
	}

	//go through second triangle, check every half edge
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
		}
	}
	// push remaining vertices of first triangle 
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

void blossom_quad::remove_unnecessary_vertices() {

	for (PolyMesh::VertexIter v_it = blossomMesh_step1.vertices_begin(); v_it != blossomMesh_step1.vertices_end(); ++v_it) {
		PolyMesh::VertexHandle vh = *v_it;
		int idx_unnecessary_vert = 0;
		int number_of_neighbours = 0;
		int idx_neighbour0 = 0;
		int idx_neighbour1 = 0;
		bool is_no_border = true;
		int idx_neighbour_face0 = -1;
		int idx_neighbour_face1 = -1;
		for (auto voh_it = blossomMesh_step1.voh_iter(vh); voh_it.is_valid(); ++voh_it) {
			PolyMesh::HalfedgeHandle heh = *voh_it;
			PolyMesh::VertexHandle neighbour = blossomMesh_step1.to_vertex_handle(heh);
			PolyMesh::FaceHandle n_face = blossomMesh_step1.face_handle(heh);
			number_of_neighbours++;
			if (number_of_neighbours == 1) {
				idx_neighbour0 = neighbour.idx();
				idx_neighbour_face0 = n_face.idx();
			}
			else if (number_of_neighbours == 2) {
				idx_neighbour1 = neighbour.idx();
				idx_neighbour_face1 = n_face.idx();
			}
			else {
				idx_neighbour0 = 0;
				idx_neighbour1 = 0;
			}
			if (n_face.idx() == -1) {
				is_no_border = false;
			}
		}
		if ( (number_of_neighbours == 2) && is_no_border ) {
			//cout << vh.idx() << endl;
			idx_unnecessary_vert = vh.idx();
			PolyMesh::FaceHandle fh0 = blossomMesh_step1.face_handle(idx_neighbour_face0);
			PolyMesh::FaceHandle fh1 = blossomMesh_step1.face_handle(idx_neighbour_face1);

			vector<PolyMesh::VertexHandle> face_vhandles;
			int v_face_0 = 0;
			//go through first triangle, check every half edge
			for (auto fh_it = blossomMesh_step1.fh_iter(fh0); fh_it.is_valid(); ++fh_it) {
				PolyMesh::HalfedgeHandle heh_q1 = *fh_it;
				PolyMesh::VertexHandle from_vert = blossomMesh_step1.from_vertex_handle(heh_q1);
				if (from_vert.idx() == idx_unnecessary_vert) {
					break;
				}
				face_vhandles.push_back(from_vert);
				v_face_0++;	
			}
			for (auto fh_it = blossomMesh_step1.fh_iter(fh1); fh_it.is_valid(); ++fh_it) {
				PolyMesh::HalfedgeHandle heh_q2 = *fh_it;
				PolyMesh::VertexHandle from_vert = blossomMesh_step1.from_vertex_handle(heh_q2);
				int from_vert_idx = from_vert.idx();
				if (from_vert_idx != idx_neighbour0 && from_vert_idx != idx_neighbour1 && from_vert_idx != idx_unnecessary_vert) {
					face_vhandles.push_back(from_vert);
					v_face_0++;
				}
			}
			for (auto fh_it = blossomMesh_step1.fh_iter(fh0); fh_it.is_valid(); ++fh_it) {
				PolyMesh::HalfedgeHandle heh = *fh_it;
				if (v_face_0 != 0) {
					// skip all already pushed vertices
					v_face_0--;
					continue;
				}
				else {
					PolyMesh::VertexHandle fvhs = blossomMesh_step1.from_vertex_handle(heh);
					face_vhandles.push_back(fvhs);
				}
			}

			// remove obsolete faces
			blossomMesh_step1.request_face_status();
			blossomMesh_step1.delete_face(fh0, false);
			blossomMesh_step1.request_face_status();
			blossomMesh_step1.delete_face(fh1, false);
			// add new face
			blossomMesh_step1.request_face_status();
			blossomMesh_step1.add_face(face_vhandles);
			// remove unnecessary vertex
			blossomMesh_step1.request_vertex_status();
			blossomMesh_step1.delete_vertex(vh, false);
			blossomMesh_step1.garbage_collection();
		}
	}
}

int blossom_quad::remove_unnecessary_faces(int point_count) {
	int temp_point_count = 10;//point_count;
	PolyMesh delete_face_test_mesh;
	std::vector<PolyMesh::VertexHandle> delete_face_test_mesh_Vec(10);
	delete_face_test_mesh_Vec[0] = delete_face_test_mesh.add_vertex(PolyMesh::Point(0, 0, 0));
	delete_face_test_mesh_Vec[1] = delete_face_test_mesh.add_vertex(PolyMesh::Point(2, 0, 0));
	delete_face_test_mesh_Vec[2] = delete_face_test_mesh.add_vertex(PolyMesh::Point(4, 0, 0));
	delete_face_test_mesh_Vec[3] = delete_face_test_mesh.add_vertex(PolyMesh::Point(0, 1, 0));
	delete_face_test_mesh_Vec[4] = delete_face_test_mesh.add_vertex(PolyMesh::Point(1, 1, 0));
	delete_face_test_mesh_Vec[5] = delete_face_test_mesh.add_vertex(PolyMesh::Point(3, 1, 0));
	delete_face_test_mesh_Vec[6] = delete_face_test_mesh.add_vertex(PolyMesh::Point(4, 1, 0));
	delete_face_test_mesh_Vec[7] = delete_face_test_mesh.add_vertex(PolyMesh::Point(0, 2, 0));
	delete_face_test_mesh_Vec[8] = delete_face_test_mesh.add_vertex(PolyMesh::Point(2, 2, 0));
	delete_face_test_mesh_Vec[9] = delete_face_test_mesh.add_vertex(PolyMesh::Point(4, 2, 0));

	delete_face_test_mesh.add_face({ delete_face_test_mesh_Vec[0], delete_face_test_mesh_Vec[1], delete_face_test_mesh_Vec[4], delete_face_test_mesh_Vec[3] });
	delete_face_test_mesh.add_face({ delete_face_test_mesh_Vec[1], delete_face_test_mesh_Vec[2], delete_face_test_mesh_Vec[6], delete_face_test_mesh_Vec[5] });
	delete_face_test_mesh.add_face({ delete_face_test_mesh_Vec[3], delete_face_test_mesh_Vec[4], delete_face_test_mesh_Vec[8], delete_face_test_mesh_Vec[7] });
	delete_face_test_mesh.add_face({ delete_face_test_mesh_Vec[5], delete_face_test_mesh_Vec[6], delete_face_test_mesh_Vec[9], delete_face_test_mesh_Vec[8] });
	delete_face_test_mesh.add_face({ delete_face_test_mesh_Vec[1], delete_face_test_mesh_Vec[5], delete_face_test_mesh_Vec[8], delete_face_test_mesh_Vec[4] });

	OpenMesh::IO::write_mesh(delete_face_test_mesh, "Created_Meshes/delete_face_test_mesh_raw.off");
	// faces to delete
	vector<PolyMesh::FaceHandle> old_faces;
	int number_of_old_faces = 0;
	vector<vector<PolyMesh::VertexHandle>> new_faces;
	int number_of_new_faces = 0;


	for (PolyMesh::FaceIter f_it = delete_face_test_mesh.faces_begin(); f_it != delete_face_test_mesh.faces_end(); ++f_it) {
		PolyMesh::FaceHandle fh = *f_it;
		vector<int> vertex_neighours;
		vector<PolyMesh::VertexHandle> face_vertices;

		// go through all vertices on a face, to find out their neighbour number
		for (auto fv_it = delete_face_test_mesh.fv_iter(fh); fv_it.is_valid(); ++fv_it) {
			PolyMesh::VertexHandle vh = *fv_it;
			face_vertices.push_back(vh);
			int vertex_n = 0;
			// count neighbour vertices
			for (auto voh_it = delete_face_test_mesh.voh_iter(vh); voh_it.is_valid(); ++voh_it) {
				PolyMesh::HalfedgeHandle heh = *voh_it;
				PolyMesh::FaceHandle ges_fh = delete_face_test_mesh.face_handle(heh);
				if (ges_fh.idx() == -1) {
					vertex_n = -1;
					break;					
				}
				else {
					vertex_n++;
				}
			}
			vertex_neighours.push_back(vertex_n);
		}
		//cout << "face ID " << fh.idx() << endl;
		//for (int i = 0; i < 4; i++) {
		//	cout << "vertex " << face_vertices[i].idx() << " has " << vertex_neighours[i] << " neighbours" << endl;
		//}
		if ((vertex_neighours[0] == 3) && (vertex_neighours[2] == 3)) {
			vec3 *point0 = new vec3(delete_face_test_mesh.point(face_vertices[0])[0], delete_face_test_mesh.point(face_vertices[0])[1], delete_face_test_mesh.point(face_vertices[0])[2]);
			vec3 *point1 = new vec3(delete_face_test_mesh.point(face_vertices[2])[0], delete_face_test_mesh.point(face_vertices[2])[1], delete_face_test_mesh.point(face_vertices[2])[2]);

			vec3 vec10 = *point1 - *point0;
			vec3 *new_point = new vec3(point0->x + 0.5 * vec10.x, point0->y + 0.5 * vec10.y, point0->z + 0.5 * vec10.z);
			delete_face_test_mesh_Vec.push_back(delete_face_test_mesh.add_vertex(PolyMesh::Point(new_point->x, new_point->y, new_point->z)));
			int index_of_new_point = temp_point_count;
			temp_point_count++;

			// get indices of surrounding faces

			old_faces.push_back(fh);
			number_of_old_faces++;
			for (auto voh_it = delete_face_test_mesh.voh_iter(face_vertices[0]); voh_it.is_valid(); ++voh_it) {
				PolyMesh::HalfedgeHandle heh = *voh_it;
				PolyMesh::FaceHandle ges_fh = delete_face_test_mesh.face_handle(heh);
				if (ges_fh.idx() != fh.idx()) {
					old_faces.push_back(ges_fh);
					number_of_old_faces++;
				}
			}
			for (auto voh_it = delete_face_test_mesh.voh_iter(face_vertices[2]); voh_it.is_valid(); ++voh_it) {
				PolyMesh::HalfedgeHandle heh = *voh_it;
				PolyMesh::FaceHandle ges_fh = delete_face_test_mesh.face_handle(heh);
				if (ges_fh.idx() != fh.idx()) {
					old_faces.push_back(ges_fh);
					number_of_old_faces++;
				}
			}

			for (int i = 0; i < 5; i++) {
				//cout << old_faces[i].idx() << endl;
				vector<PolyMesh::VertexHandle> new_face;
				if (old_faces[number_of_old_faces - 5 + i].idx() != fh.idx()) {
					for (auto old_fv_it = delete_face_test_mesh.fv_iter(old_faces[number_of_old_faces - 5 + i]); old_fv_it.is_valid(); ++old_fv_it) {
						PolyMesh::VertexHandle old_vh = *old_fv_it;
						if ((old_vh.idx() == face_vertices[0].idx()) || (old_vh.idx() == face_vertices[2].idx())) {
							new_face.push_back(delete_face_test_mesh_Vec[index_of_new_point]);
						}
						else {
							new_face.push_back(old_vh);
						}
					}
					new_faces.push_back(new_face);
					number_of_new_faces++;
				}
			}
		}
		else if ((vertex_neighours[1] == 3) && (vertex_neighours[3] == 3)) {
			vec3 *point0 = new vec3(delete_face_test_mesh.point(face_vertices[1])[0], delete_face_test_mesh.point(face_vertices[1])[1], delete_face_test_mesh.point(face_vertices[1])[2]);
			vec3 *point1 = new vec3(delete_face_test_mesh.point(face_vertices[3])[0], delete_face_test_mesh.point(face_vertices[3])[1], delete_face_test_mesh.point(face_vertices[3])[2]);

			vec3 vec10 = *point1 - *point0;
			vec3 *new_point = new vec3(point0->x + 0.5 * vec10.x, point0->y + 0.5 * vec10.y, point0->z + 0.5 * vec10.z);
			delete_face_test_mesh_Vec.push_back(delete_face_test_mesh.add_vertex(PolyMesh::Point(new_point->x, new_point->y, new_point->z)));
			int index_of_new_point = temp_point_count;
			temp_point_count++;

			// get indices of surrounding faces
			
			old_faces.push_back(fh);
			number_of_old_faces++;
			for (auto voh_it = delete_face_test_mesh.voh_iter(face_vertices[1]); voh_it.is_valid(); ++voh_it) {
				PolyMesh::HalfedgeHandle heh = *voh_it;
				PolyMesh::FaceHandle ges_fh = delete_face_test_mesh.face_handle(heh);
				if (ges_fh.idx() != fh.idx()) {
					old_faces.push_back(ges_fh);
					number_of_old_faces++;
				}
			}
			for (auto voh_it = delete_face_test_mesh.voh_iter(face_vertices[3]); voh_it.is_valid(); ++voh_it) {
				PolyMesh::HalfedgeHandle heh = *voh_it;
				PolyMesh::FaceHandle ges_fh = delete_face_test_mesh.face_handle(heh);
				if (ges_fh.idx() != fh.idx()) {
					old_faces.push_back(ges_fh);
					number_of_old_faces++;
				}
			}
			
			for (int i = 0; i < 5; i++) {
				//cout << old_faces[i].idx() << endl;
				vector<PolyMesh::VertexHandle> new_face;
				if (old_faces[number_of_old_faces - 5 + i].idx() != fh.idx()) {
					for (auto old_fv_it = delete_face_test_mesh.fv_iter(old_faces[number_of_old_faces - 5 + i]); old_fv_it.is_valid(); ++old_fv_it) {
						PolyMesh::VertexHandle old_vh = *old_fv_it;
						if ((old_vh.idx() == face_vertices[1].idx()) || (old_vh.idx() == face_vertices[3].idx())) {
							new_face.push_back(delete_face_test_mesh_Vec[index_of_new_point]);
						}
						else {
							new_face.push_back(old_vh);
						}
					}
					new_faces.push_back(new_face);
					number_of_new_faces++;
				}	
			}			
		}
	}
	for (int k = 0; k < number_of_old_faces; k++) {
		cout << old_faces[k].idx() << endl;
		delete_face_test_mesh.request_face_status();
		delete_face_test_mesh.delete_face(old_faces[k], false);
	}
	for (int l = 0; l < number_of_new_faces; l++) {
		delete_face_test_mesh.request_face_status();
		delete_face_test_mesh.add_face(new_faces[l]);
	}
	delete_face_test_mesh.garbage_collection();
	OpenMesh::IO::write_mesh(delete_face_test_mesh, "Created_Meshes/delete_face_test_mesh.off");

	return temp_point_count;
}

float blossom_quad::scalar_product(glm::vec3 vector1, vec3 vector2) {
	return (vector1[0] * vector2[0] + vector1[1] * vector2[1] + vector1[2] * vector2[2]);
}

float blossom_quad::vec_length(vec3 vector1) {
	return sqrt(((float)vector1[0]) * ((float)vector1[0]) + ((float)vector1[1]) * ((float)vector1[1]) + ((float)vector1[2]) * ((float)vector1[2]));
}