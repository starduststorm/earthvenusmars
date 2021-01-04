#ifndef LEDGRAPH_H
#define LEDGRAPH_H

#include <vector>
#include <algorithm>
#include <FastLED.h>
#include <set>
#include <util.h>

using namespace std;

struct Edge {
    typedef enum {
        none             = 0,
        inbound          = 1 << 0,
        outbound         = 1 << 1,
        clockwise        = 1 << 2,
        counterclockwise = 1 << 3
    } EdgeType;
    
    int from, to; 
    EdgeType type;
    Edge(int from, int to, EdgeType type) : from(from), to(to), type(type) {};
    Edge transpose() {
        EdgeType transposeType;
        switch (type) {
            case inbound: transposeType = outbound; break;
            case outbound: transposeType = inbound; break;
            case clockwise: transposeType = counterclockwise; break;
            case counterclockwise: transposeType = clockwise; break;
            default: transposeType = none; break;
        }
        return Edge(to, from, transposeType);
    }
};

typedef Edge::EdgeType EdgeType;
typedef uint8_t EdgeTypes;
unsigned EdgeTypeCount = 4;

class Graph {
public:
    vector<vector<Edge> > adjList;
    Graph() { }
    Graph(vector<Edge> const &edges, int count) {
        adjList.resize(count);

        for (auto &edge : edges) {
            addEdge(edge);
        }
    }

    void addEdge(Edge edge, bool bidirectional=true) {
        adjList[edge.from].push_back(edge);
        if (bidirectional) {
            adjList[edge.to].push_back(edge.transpose());
        }
    }

    vector<Edge> adjacencies(int vertex, EdgeTypes matching = 0) {
        vector<Edge> adjacencies = vector<Edge>(adjList[vertex]);
        if (matching) {
            for (auto it = adjacencies.begin(); it != adjacencies.end();) {
                if (!(matching & it->type)) {
                    adjacencies.erase(it);
                } else {
                    ++it;
                }
            }
        }
        return adjacencies;
    }
};

#define NUM_LEDS (78)

Graph ledgraph;

vector<int> circleleds = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 30, 31, 32, 33, 34, 35, 36, 37, 38, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65}; // 34
vector<int> leafleds = {22, 23, 29, 26, 49, 50, 76, 75, 73};

vector<int> venusleds = {66, 67, 68, 69, 70, 71, 72, 73, 77, 76, 74, 75}; // 12
vector<int> marsleds = {39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 52, 51, 50}; // 14
vector<int> earthleds = {12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 25, 24, 23, 28, 29, 27, 26}; // 18
vector<int> earthasvenusleds = {12, 13, 14, 15, 16, 17, 18, 19, 28, 29, 27, 26}; // 12
vector<int> earthasmarsleds = {12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 25, 24, 23}; // 14

void initLEDGraph() {
    vector<Edge> edges = {
        // spoke connections
        {30, 12, Edge::outbound}, {38, 39, Edge::outbound}, {65,66, Edge::outbound},

        // earth arrow
        {19,20, Edge::outbound}, {20,21, Edge::outbound}, {21,22, Edge::outbound},
        {19,25, Edge::outbound}, {25,24, Edge::outbound}, {24,23, Edge::outbound},
        
        // earth cross
        {14,28, Edge::outbound}, {28, 29, Edge::outbound},
        {14,27, Edge::outbound}, {27, 26, Edge::outbound},

        // mars arrow
        {46, 47, Edge::outbound}, {47, 48, Edge::outbound}, {48, 49, Edge::outbound},
        {46, 52, Edge::outbound}, {52, 51, Edge::outbound}, {51, 50, Edge::outbound},

        // venus cross
        {70, 77, Edge::outbound}, {77, 76, Edge::outbound},
        {70, 74, Edge::outbound}, {74, 75, Edge::outbound},
    };

    ledgraph = Graph(edges, NUM_LEDS);

    // circle is pixels D1-D12, D31-D39, D54-D66
    // e.g. leds[] 0-11, 30-38, 53-65
    for (unsigned i = 0; i < circleleds.size(); ++i) {
        ledgraph.addEdge(Edge(circleleds[i], circleleds[mod_wrap(i+1, circleleds.size())], Edge::clockwise));
    }

    // spokes are D13-D20, D40-D47, D67-D74
    // e.g. leds[] 12-19, 39-46, 66-73
    for (int i = 12; i < 19; ++i) {
        ledgraph.addEdge(Edge(i, i+1, Edge::outbound));
    }
    for (int i = 39; i < 46; ++i) {
        ledgraph.addEdge(Edge(i, i+1, Edge::outbound));
    }
    for (int i = 66; i < 73; ++i) {
        ledgraph.addEdge(Edge(i, i+1, Edge::outbound));
    }
}

#endif
