#ifndef LEDGRAPH_H
#define LEDGRAPH_H

#include <vector>
#include <algorithm>
#include <FastLED.h>
#include <set>
#include <util.h>

#include "drawing.h"

using namespace std;

const uint8_t EdgeTypesCount = 4;

typedef union {
  struct {
    uint8_t first:4;
    uint8_t second:4;
  } edgeTypes;
  uint8_t pair;
} EdgeTypesPair;

struct Edge {
    typedef enum : uint8_t {
        none             = 0,
        inbound          = 1 << 0,
        outbound         = 1 << 1,
        clockwise        = 1 << 2,
        counterclockwise = 1 << 3,
        all              = 0xFF,
    } EdgeType;
    
    uint8_t from, to;
    EdgeType type;
    Edge(uint8_t from, uint8_t to, EdgeType type) : from(from), to(to), type(type) {};
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

EdgeTypesPair MakeEdgeTypesPair(vector<EdgeTypes> vec) {
    assert(vec.size() <= 2, "only two edge type directions allowed");
    unsigned size = vec.size();
    EdgeTypesPair pair = {0};
    if (size > 0) {
        pair.edgeTypes.first = vec[0];
    }
    if (size > 1) {
        pair.edgeTypes.second = vec[1];
    }
    return pair;
}

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

    vector<Edge> adjacencies(uint8_t vertex, EdgeTypesPair pair) {
        vector<Edge> adjList;
        getAdjacencies(vertex, pair.edgeTypes.first, adjList);
        getAdjacencies(vertex, pair.edgeTypes.second, adjList);
        return adjList;
    }

    void getAdjacencies(uint8_t vertex, EdgeTypes matching, std::vector<Edge> &insertInto) {
        if (matching == 0) {
            return;
        }
        vector<Edge> &adj = adjList[vertex];
        for (Edge &edge : adj) {
            if (edge.type & matching) {
                insertInto.push_back(edge);
            }
        }
    }
};

#define NUM_LEDS (78)

Graph ledgraph;

#define CIRCLE_LEDS 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 30, 31, 32, 33, 34, 35, 36, 37, 38, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65 // 34

#define EARTH_LEDS 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 25, 24, 23, 28, 29, 27, 26 // 18
#define VENUS_LEDS 66, 67, 68, 69, 70, 71, 72, 73, 77, 76, 74, 75 // 12
#define MARS_LEDS 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 52, 51, 50 // 14

const vector<uint8_t> circleleds = {CIRCLE_LEDS};
const vector<uint8_t> leafleds = {22, 23, 29, 26, 49, 50, 76, 75, 73};
const vector<uint8_t> spoke_tip_leds = {19, 46, 73};
const vector<uint8_t> spoke_base_leds = {12, 39, 68};

const vector<uint8_t> venusleds = {VENUS_LEDS};
const vector<uint8_t> marsleds = {MARS_LEDS};

const vector<uint8_t> earthleds = {EARTH_LEDS};
const vector<uint8_t> earthasvenusleds = {12, 13, 14, 15, 16, 17, 18, 19, 28, 29, 27, 26}; // 12
const vector<uint8_t> earthasmarsleds = {12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 25, 24, 23}; // 14

// FIXME: make these vectors? I expect the asymptotic win from searching a ~50-element std::set is not enough to counter the constant from following pointers + the increased memory usage
const set<uint8_t> circleEarthLeds = {CIRCLE_LEDS, EARTH_LEDS};
const set<uint8_t> circleVenusLeds = {CIRCLE_LEDS, VENUS_LEDS};
const set<uint8_t> circleMarsLeds = {CIRCLE_LEDS, MARS_LEDS};

// indexes into circleleds vector opposite each spoke for use in pathing
const uint8_t circleIndexOppositeEarth = 29;
const uint8_t circleIndexOppositeVenus = 16;
const uint8_t circleIndexOppositeMars = 3;

static const set<uint8_t> *const kSpokeCircleLedSets[] = {&circleEarthLeds, &circleVenusLeds, &circleMarsLeds};
static const vector<uint8_t> * const kSpokeLedLists[] = {&earthleds, &venusleds, &marsleds};


// shortcuts for quickly determined if a pixel is in a given spoke
bool onEarth(uint8_t px) {
    return px >= 12 && px <= 29;
}

bool onVenus(uint8_t px) {
    return px >= 66 && px <= 77;
}

bool onMars(uint8_t px) {
    return px >= 39 && px <= 52;
}

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
    for (uint8_t i = 12; i < 19; ++i) {
        ledgraph.addEdge(Edge(i, i+1, Edge::outbound));
    }
    for (uint8_t i = 39; i < 46; ++i) {
        ledgraph.addEdge(Edge(i, i+1, Edge::outbound));
    }
    for (uint8_t i = 66; i < 73; ++i) {
        ledgraph.addEdge(Edge(i, i+1, Edge::outbound));
    }
}

typedef CustomDrawingContext<NUM_LEDS, 1, CRGB, CRGBArray<NUM_LEDS> > EVMDrawingContext;

#endif
