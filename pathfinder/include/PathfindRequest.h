//
// Created by syeyoung on 2024/02/26.
//

#ifndef PATHFINDER2_PATHFINDREQUEST_H
#define PATHFINDER2_PATHFINDREQUEST_H

#include <cstdint>
#include <string>
#include <vector>
#include "CollisionState.h"

typedef struct {
    bool enderpearl;
    bool tntpearl;
    bool stonkdown;
    bool stonkechest;
    bool stonkteleport;
    bool etherwarp;
    uint32_t maxStonkLen;
    int etherwarpRadius;
    float etherwarpLeeway;
    float etherwarpOffset;
} AlgorithmSettings;


enum PearlLandState:  uint8_t {
    PEARL_LAND_STATE_FLOOR = 0,
    PEARL_LAND_STATE_CEILING = 1,
    PEARL_LAND_STATE_FLOOR_WALL = 2,
    PEARL_LAND_STATE_WALL = 3,
    PEARL_LAND_STATE_BLOCKED = 4,
    PEARL_LAND_STATE_OPEN = 5
};

struct Coordinate {
    int x;
    int y;
    int z;
    bool operator>(const Coordinate& other) const {
        return x > other.x || (x == other.x && y > other.y) || (x == other.x && y == other.y && z > other.z);
    }
    bool operator<(const Coordinate& other) const {
        return x < other.x || (x == other.x && y < other.y) || (x == other.x && y == other.y && z < other.z);
    }
};


typedef struct {
    bool isInsta;
    PearlLandState pearl;
    CollisionState data;
} OctNode;

typedef struct {
    uint8_t id;
    uint8_t data;
} Block;

struct BlockWorld {
    int xLen;
    int zLen;
    int yLen;
    std::vector<Block> blocks;
    Block getBlock(int x, int y, int z) {
        if (x < 0 || y < 0 || z < 0) return {  7, 0 };
        if (x >= xLen || z >= zLen || y >= yLen)  return { 7 ,0 };

        return blocks[y * xLen * zLen + z * xLen + x];
    }
    Block getBlock(Coordinate& c) {
        if (c.x < 0 || c.y < 0 || c.z < 0) return { 7,0 };
        if (c.x >= xLen || c.z >= zLen || c.y >= yLen)  return {7,0 };

        return blocks[c.y * xLen * zLen + c.z * xLen + c.x];
    }
};

struct OctNodeWorld {
    int xLen;
    int zLen;
    int yLen;
    std::vector<OctNode> nodes;

    OctNode getOctNode(int  x, int  y, int  z) {
        if (x < 0 || y < 0 || z < 0) return { false, PEARL_LAND_STATE_BLOCKED, COLLISION_STATE_BLOCKED };
        if (x >= xLen || z >= zLen || y >= yLen)  return { false, PEARL_LAND_STATE_BLOCKED, COLLISION_STATE_BLOCKED };

        return nodes[y * xLen * zLen + z * xLen + x];
    }
    OctNode getOctNode(Coordinate& c) {
        if (c.x < 0 || c.y < 0 || c.z < 0) return { false, PEARL_LAND_STATE_BLOCKED, COLLISION_STATE_BLOCKED };
        if (c.x >= xLen || c.z >= zLen || c.y >= yLen)  return { false, PEARL_LAND_STATE_BLOCKED, COLLISION_STATE_BLOCKED };

        return nodes[c.y * xLen * zLen + c.z * xLen + c.x];
    }
};

class PathfindRequest {
public:
    std::string id;
    std::string uuid;
    std::string name;
    AlgorithmSettings settings;
    std::vector<Coordinate> target;
    BlockWorld blockWorld;
    OctNodeWorld octNodeWorld;

    void ReadRequest(std::istream& file);
};


#endif //PATHFINDER2_PATHFINDREQUEST_H
