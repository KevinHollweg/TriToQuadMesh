#include "graph_classes.h"

graph_edge::graph_edge() {}

graph_edge::~graph_edge() {}


graph::graph() {}

graph::~graph() {}

using namespace std;

void graph::print_graph() {
	for (int i = 0; i < edges_counter; i++) {
		if (!edges[i].bad) {
			cout << "edge between face " << edges[i].face0.idx() << " and face " << edges[i].face1.idx() << " with cost " << edges[i].cost << endl;
		}	
	}
	for (int j = 0; j < external_edges_counter; j++) {
		if (!external_edges[j].bad) {
			cout << "external edge between face " << external_edges[j].face0.idx() << " and face " << external_edges[j].face1.idx() << " with cost " << external_edges[j].cost << " over vertex " << external_edges[j].connecting_vertex.idx() << endl;
		}
	}
}