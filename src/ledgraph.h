#ifndef LEDGRAPH_H
#define LEDGRAPH_H

#include <vector>
#include <FastLED.h>
#include <set>
#include <util.h>

using namespace std;

struct Edge { int from, to; };

class Graph {
public:
    vector<vector<int> > adjList;

    Graph() { }
    Graph(vector<Edge> const &edges, int count) {
        adjList.resize(count);

        for (auto &edge : edges) {
            addEdge(edge.from, edge.to);
        }
    }

    void addEdge(int from, int to) {
        adjList[from].push_back(to);
        adjList[to].push_back(from);
    }

    const vector<int> adjacent_to(int vertex) {
        return adjList[vertex];
    }
};

#define NUM_LEDS (78)

Graph ledgraph;

int circleleds[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 30, 31, 32, 33, 34, 35, 36, 37, 38, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65};
// These only include a nearby arc, not the full circle
int venusleds[] = {0, 1, 2, 3, 65, 64, 63, 62, 61, 66, 67, 68, 69, 70, 71, 72, 73, 77, 76, 74, 75};
int marsleds[] = {38, 37, 36, 35, 34, 53, 54, 55, 56, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 52, 51, 50};
int earthleds[] = {30, 11, 10, 9, 8, 31, 32, 33, 34, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 25, 24, 23, 28, 29, 27, 26};

void initLEDGraph() {
    vector<Edge> edges = {
        // spoke connections
        {30, 12}, {38, 39}, {65,66},

        // earth arrow
        {19,20}, {20,21}, {21,22},
        {19,25}, {25,24}, {24,23},
        
        // earth cross
        {14,28}, {28, 29},
        {14,27}, {27, 26},

        // mars arrow
        {45, 47}, {47, 48}, {48, 49},
        {45, 52}, {52, 51}, {51, 50},

        // venus cross
        {70, 77}, {77, 76},
        {70, 74}, {74, 75},
    };

    ledgraph = Graph(edges, NUM_LEDS);

    logf("adding circle edges");
    // circle is pixels D1-D12, D31-D39, D54-D66
    // e.g. leds[] 0-11, 30-38, 53-65
    for (unsigned i = 0; i < ARRAY_SIZE(circleleds); ++i) {
        ledgraph.addEdge(circleleds[i], circleleds[(i+1) % ARRAY_SIZE(circleleds)]);
    }

    // spokes are D13-D20, D40-D47, D67-D74
    // e.g. leds[] 12-19, 39-46, 66-73
    for (int i = 12; i < 19; ++i) {
        ledgraph.addEdge(i, i+1);
    }
    for (int i = 39; i < 46; ++i) {
        ledgraph.addEdge(i, i+1);
    }
    for (int i = 66; i < 73; ++i) {
        ledgraph.addEdge(i, i+1);
    }
}

#endif
