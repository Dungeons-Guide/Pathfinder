//
// Created by syeyoung on 2024/02/26.
//

#ifndef PATHFINDER2_PATHFINDER_H
#define PATHFINDER2_PATHFINDER_H

#include "PathfindRequest.h"
#include "ShadowCaster.h"
#include <limits>

enum ConnectionType : uint8_t {
    ConnectionType_STONK_EXIT = 0,
    ConnectionType_STONK_WALK, // 1
    ConnectionType_WALK, // 2
    ConnectionType_ETHERWARP, // 3
    ConnectionType_DIG_DOWN,
    ConnectionType_ECHEST,
    ConnectionType_DIG_UP,
    ConnectionType_TELEPORT_INTO,
    ConnectionType_SUPERBOOM,
    ConnectionType_TNTPEARL,
    ConnectionType_ENDERPEARL,
    ConnectionType_TARGET,
    ConnectionType_UNPOPULATED
} ;

struct PathfindNode {
    Coordinate parent = {0,0,0};
    float gScore = std::numeric_limits<float>::max();
    uint8_t stonkLen = 0;
    ConnectionType type = ConnectionType_UNPOPULATED;
} ;

class Pathfinder {
private:
public:
    ShadowCaster shadowCaster;
    PathfindRequest& request;
    std::vector<std::vector<std::vector<PathfindNode> > > nodes;
    std::vector<std::vector<std::vector<std::vector<Coordinate>>>> etherwarps;
    int minY;
    int maxY;

    void Populate();
    PathfindNode& GetNode(int x, int  y, int  z);
    PathfindNode& GetNode(const Coordinate& coord);


    explicit Pathfinder(PathfindRequest& request);
};



#endif //PATHFINDER2_PATHFINDER_H
