//
// Created by syeyoung on 2024/02/26.
//

#include "../include/Pathfinder.h"
#include <queue>
#include <iostream>
#include <set>
#include <cmath>


using namespace std;


typedef pair<float, Coordinate> NodePair;

PathfindNode& Pathfinder::GetNode(int  x, int  y, int  z) {
    return nodes[x+5][z+5][y+5 - minY];
}
PathfindNode& Pathfinder::GetNode(const Coordinate& coord) {
    return nodes[coord.x+5][coord.z+5][coord.y+5 - minY];
}

bool emptyFor(Block b) {
    if (b.id == 30) return true; // can't warp on cobweb.
    if (b.id == 0) return true; // air
    if (b.id == 171) return true; // carpet
    if (b.id == 144) return true; // skull
    if (b.id == 63 || b.id == 68) return true; // sign
    if (8 <= b.id && b.id <= 11) return true; // fluids
    if (b.id == 176 || b.id == 177) return true; // banner
    if (b.id == 70 || b.id == 72 || b.id == 147 || b.id == 148) return true;
    if (b.id == 6 || b.id == 37 || b.id == 38 || b.id == 39 || b.id == 40 ||
        b.id == 31 || b.id == 32 || b.id == 175 || b.id == 104 || b.id == 105 ||
        b.id == 141 || b.id == 142 || b.id == 115 || b.id == 59) return true;
    //plants: 1. sapling 2. dandellion 3. rest of flower 4. brown mushroom 5. red mushroom
    //        6. grass/fern 7. deadbush 8. float plant 9. pumpkin stem 10. melon stem
    //        11. potato 12. carrot 13. wart 14. wheat
    if (b.id == 77 || b.id == 143) return true; // buttons, stone and wood
    if (b.id == 51) return true; // fire
    if (b.id == 69) return true; // lever;
    if (b.id == 90) return true; // portal
    if (b.id == 27 || b.id == 28 || b.id == 66 || b.id == 157) return true; // powah , detc, normal, activator rail
    if (b.id == 55) return true; // redstone wire
    if (b.id == 83) return true; // sugarcane
    if (b.id == 50 || b.id == 75 || b.id == 76) return true; //torches. normal, unlit redstone, lit redstone
    if (b.id == 131 || b.id == 132) return true; // hook and tripwire
    if (b.id == 106) return true; // vine.
    return false;
}

int FACINGS[6][3] = {
        {1,0,0},
        {-1,0,0},
        {0,1,0},
        {0,-1,0},
        {0,0,1},
        {0,0,-1}
};

int _distSq(int dx, int dy, int dz) {
    return dx * dx + dy * dy + dz * dz;
}

void Pathfinder::Populate() {
    priority_queue<NodePair, vector<NodePair>, greater<NodePair>> pq;

    for (const auto &item: request.target) {
        NodePair pair = make_pair(0, item);
        pq.push(pair);
        PathfindNode& node = GetNode(item);
        node.gScore = 0.0;
        node.type = ConnectionType_TARGET;
    }
    std::vector<Coordinate> shadowcastRes;
    shadowcastRes.reserve(100000);

    while (!pq.empty()) {
        // explore.
        auto scoreitthinks = pq.top().first;
        auto coord = pq.top().second;
        pq.pop();
        PathfindNode& n = GetNode(coord);
        if (n.gScore < scoreitthinks) {
            continue;
        }

        auto octNode = request.octNodeWorld.getOctNode(coord);

        auto originNodeState = octNode.data;
        auto originalPearlType = octNode.pearl;


        if (isBlocked(originNodeState) && request.settings.stonkteleport
            && coord.x % 2 != 0 && coord.z % 2 != 0 && coord.y % 2 == 0) {
            Block b = request.blockWorld.getBlock((coord.x-1) / 2, coord.y / 2 - 1, (coord.z-1) / 2);
            Block b2 = request.blockWorld.getBlock((coord.x-1) / 2, coord.y / 2, (coord.z-1) / 2);
            if (b.id == 85 || b.id == 113 || (188 <= b.id && b.id <= 192) || b.id == 139) { // first 3: stairs, last one: cobblestone wall
                if (b2.id == 0) {
                    Coordinate neighborCoordinate = {
                            coord.x, coord.y + 1, coord.z
                    };
                    PathfindNode *neighbor = &GetNode(neighborCoordinate);
                    auto neighborState = request.octNodeWorld.getOctNode(neighborCoordinate).data;
                    if (neighborCoordinate.y <= minY - 5) continue;
                    if (neighborCoordinate.y >= maxY + 5) continue;
                    if (!isBlocked(neighborState) && coord.y > minY - 5 && coord.y < maxY + 5) {
                        float gScore = n.gScore + 4;
                        if (gScore < neighbor->gScore) {
                            neighbor->parent = coord;
                            neighbor->stonkLen = 0;
                            neighbor->type = ConnectionType_TELEPORT_INTO;
                            neighbor->gScore= gScore;

                            pq.emplace(neighbor->gScore, neighborCoordinate);
                        }
                    }
                }
            }
        }
        if (request.settings.etherwarp
            && (coord.x % 2) != 0 && (coord.z % 2) != 0 && coord.y % 2 == 0 && coord.y > 2) {
            Block b = request.blockWorld.getBlock((coord.x-1) / 2, coord.y / 2 - 1, (coord.z-1) / 2);
            Block b2 = request.blockWorld.getBlock((coord.x-1) / 2, coord.y / 2, (coord.z-1) / 2);
            Block b3 = request.blockWorld.getBlock((coord.x-1) / 2, coord.y / 2 + 1, (coord.z-1) / 2);
            if (!emptyFor(b) && emptyFor(b2) && emptyFor(b3) && b.id != 153) {
                // elligible for etherwarp.

                Coordinate start = Coordinate{(coord.x-1) / 2,
                                              coord.y / 2 - 1,
                                              (coord.z-1) / 2};

                shadowcastRes.clear();
                // shadow casting~
                RealShadowCast(shadowcastRes, start, request.settings.etherwarpRadius);
                for (Coordinate target : shadowcastRes) {
                    if (_distSq(target.x - coord.x, target.y - coord.y - 1, target.z - coord.z) >61 * 61) continue;
                    if (target.x < 2) continue;
                    if (target.y -3 < minY - 5) continue;
                    if (target.z < 2) continue;
                    if (target.x >= request.octNodeWorld.xLen ) continue;
                    if (target.y -3 >= maxY + 5) continue;
                    if (target.z  >= request.octNodeWorld.zLen ) continue;

                    target.y -= 3;

                    PathfindNode *neighbor = &GetNode(target);
                    auto neighborState = request.octNodeWorld.getOctNode(target).data;
                    if (!isOnGround(neighborState)) continue;

                    float gScore = n.gScore+ 20; // don't etherwarp unless it saves like 10 blocks
                    if (gScore < neighbor->gScore) {
                        neighbor->parent = coord;
                        neighbor->stonkLen = 0;
                        neighbor->type = ConnectionType_ETHERWARP;
                        neighbor->gScore= gScore;

                        pq.emplace(neighbor->gScore, target);
                    }
                }
            }
        }

        if ((originalPearlType == PEARL_LAND_STATE_FLOOR ||originalPearlType == PEARL_LAND_STATE_CEILING) && request.settings.enderpearl && isBlocked(originNodeState)) {
            for (int facingidx = 0; facingidx < 6; facingidx++) {
                if (facingidx == 2) continue;;
                bool  elligible = true;
                for (int i = 1; i < 3; i++) {
                    auto landType = request.octNodeWorld.getOctNode(coord.x + i* FACINGS[facingidx][0], coord.y + i*FACINGS[facingidx][1], coord.z + i*FACINGS[facingidx][2]).pearl;
                    if (landType != PEARL_LAND_STATE_OPEN
                        && !(originalPearlType == PEARL_LAND_STATE_FLOOR && landType == PEARL_LAND_STATE_FLOOR)
                        && !(originalPearlType == PEARL_LAND_STATE_CEILING && landType == PEARL_LAND_STATE_CEILING)) {
                        elligible = false;
                        break;
                    }
                }
                if (!elligible) continue;


                Coordinate neighborCoordinate = {
                        coord.x + FACINGS[facingidx][0] * 2, coord.y + FACINGS[facingidx][1] * 2,
                        coord.z + FACINGS[facingidx][2] * 2
                };
                PathfindNode *neighbor = &GetNode(neighborCoordinate);
                auto neighborState = request.octNodeWorld.getOctNode(neighborCoordinate).data;

                int down  =0;
                while (!isOnGround(neighborState) && neighborCoordinate.y > minY - 5) {
                    neighborCoordinate.y -= 1;
                    neighbor = &GetNode(neighborCoordinate);
                    neighborState = request.octNodeWorld.getOctNode(neighborCoordinate).data;
                    down ++;
                }

                if (neighborCoordinate.y <= minY - 5) continue;
                if (neighborCoordinate.y >= maxY + 5) continue;

                if (!isBlocked(neighborState) && down < 10) {
                    float gScore = n.gScore+ 20 + sqrt(down*down + 16);
                    if (gScore < neighbor->gScore) {
                        neighbor->parent = coord;
                        neighbor->stonkLen = 0;
                        neighbor->type = ConnectionType_ENDERPEARL;
                        neighbor->gScore= gScore;

                        pq.emplace(neighbor->gScore, neighborCoordinate);
                    }
                }
            }
        }

        if ((originalPearlType == PEARL_LAND_STATE_WALL) && request.settings.enderpearl && isBlocked(originNodeState) && isOnGround(originNodeState)) {
            for (int facingidx = 0; facingidx < 6; facingidx++) {
                if (facingidx == 2) continue;
                bool ellgible = true;
                for (int i = 1; i < 2; i++) {
                    OctNode node = request.octNodeWorld.getOctNode(coord.x + i* FACINGS[facingidx][0], coord.y + i*FACINGS[facingidx][1], coord.z + i*FACINGS[facingidx][2]);
                    auto landType = node.pearl;
                    auto collisionState = node.data;
                    if (landType != PEARL_LAND_STATE_OPEN) { ellgible = false; break; }
                    if (!isBlocked(collisionState) || !isCanGo(collisionState))  { ellgible = false; break; }
                }
                if (!ellgible) continue;


                Coordinate neighborCoordinate = {
                        coord.x + FACINGS[facingidx][0] * 2, coord.y + FACINGS[facingidx][1] * 2,
                        coord.z + FACINGS[facingidx][2] * 2
                };
                PathfindNode *neighbor = &GetNode(neighborCoordinate);
                auto neighborState = request.octNodeWorld.getOctNode(neighborCoordinate).data;
                
                int down  =0;
                while (!isOnGround(neighborState) && neighborCoordinate.y > minY - 5) {
                    neighborCoordinate.y -= 1;
                    neighbor = &GetNode(neighborCoordinate);
                    neighborState = request.octNodeWorld.getOctNode(neighborCoordinate).data;
                    down ++;
                }
                if (neighborCoordinate.y <= minY - 5) continue;
                if (neighborCoordinate.y >= maxY + 5) continue;
    
    
                if (!isBlocked(neighborState) && 5 < down && down < 30) {
                    float gScore = n.gScore+ 20 + sqrt(down*down + 16);
                    if (gScore < neighbor->gScore) {
                        neighbor->parent = coord;
                        neighbor->stonkLen = 0;
                        neighbor->type = ConnectionType_ENDERPEARL;
                        neighbor->gScore= gScore;

                        pq.emplace(neighbor->gScore, neighborCoordinate);
                    }
                }
            }
        }


//        if (isCanGo(originNodeState)) {
        if (isBlocked(originNodeState)) {
            // in wall
//            bool ontop = request.octNodeWorld.getOctNode(coord.x, coord.y + 1, coord.z).data == COLLISION_STATE_ONGROUND;
            for (int facingidx = 0; facingidx < 6; facingidx++) {
                Coordinate neighborCoordinate = {
                        coord.x + FACINGS[facingidx][0], coord.y + (facingidx == 3 ? 2 : 1) * FACINGS[facingidx][1], coord.z + FACINGS[facingidx][2]
                };
                PathfindNode *neighbor = &GetNode(neighborCoordinate);
                auto neighborState = request.octNodeWorld.getOctNode(neighborCoordinate).data;

                if (!isCanGo(neighborState) && (isOnGround(neighborState) || facingidx != 2)) {
                    continue; // obv, it's forbidden.
                }
                if (facingidx != 2 && facingidx != 3 && !isOnGround(neighborState)) {
                    continue; // you need to keep falling
                }
                if (facingidx == 3 && !isOnGround(neighborState)) {
                    continue; // can not jump while floating in air.
                }
//                    if (neighborState)

                bool elligibleForTntPearl = request.settings.tntpearl && isOnGround(neighborState) && !isClip(neighborState)
                                               && facingidx != 2 && facingidx != 3 && neighborCoordinate.y % 2 == 0 && originalPearlType == PEARL_LAND_STATE_FLOOR_WALL
                                               && request.blockWorld.getBlock((int) floor(neighborCoordinate.x / 2.0), neighborCoordinate.y / 2, (int)floor(neighborCoordinate.z / 2.0)).id == 0;


                if (!isClip(neighborState) && !elligibleForTntPearl) {
                    continue; // can not go from non-clip to blocked.
                }

                if (isBlocked(neighborState) && n.stonkLen + (facingidx == 3 ? 2 : 1) > request.settings.maxStonkLen)
                    continue;
                if (neighborState == COLLISION_STATE_ENDERCHEST && !request.settings.stonkechest)
                    continue;
                if (neighborState == COLLISION_STATE_STAIR && !request.settings.stonkdown) continue;
                if (neighborCoordinate.y < minY - 5) continue;
                if (neighborCoordinate.y >= maxY + 5) continue;


                float gScore = n.gScore;
                if (!isClip(neighborState) && elligibleForTntPearl)
                    gScore += 20; // tntpearl slow
                if (!isBlocked(neighborState) && isClip(neighborState)) {
                    // stonk entrance!!!
                    gScore += neighborState == COLLISION_STATE_ENDERCHEST ? 50 : 6; // don't enderchest unless it saves like 25 blocks
                } else if (facingidx == 3) {
                    gScore += 100;
                } else if (neighborCoordinate.x % 2 == 0 || neighborCoordinate.z % 2 == 0) {
                    gScore += 3;// pls don't jump.
                } else {
                    gScore += 2;
                }

                if (gScore < neighbor->gScore) {
                    neighbor->parent = coord;
                    if (isBlocked(neighborState))
                        neighbor->stonkLen = (n.stonkLen + (facingidx == 3 ? 2 : 1));
                    else
                        neighbor->stonkLen = 0;
                    if (neighborState == COLLISION_STATE_ENDERCHEST)
                        neighbor->type = ConnectionType_ECHEST;
                    else if (neighborState == COLLISION_STATE_STAIR)
                        neighbor->type = ConnectionType_DIG_DOWN;
                    else if (elligibleForTntPearl)
                        neighbor->type = ConnectionType_TNTPEARL;
                    else
                        neighbor->type = ConnectionType_STONK_WALK;
                    neighbor->gScore= gScore;

                    pq.emplace(neighbor->gScore, neighborCoordinate);
                }
            }
        } else {
            for (int facingidx = 0; facingidx < 6; facingidx++) {
                Coordinate neighborCoordinate = {
                        coord.x + FACINGS[facingidx][0], coord.y + FACINGS[facingidx][1], coord.z + FACINGS[facingidx][2]
                };
                PathfindNode *neighbor = &GetNode(neighborCoordinate);
                auto neighborState = request.octNodeWorld.getOctNode(neighborCoordinate).data;

                if (!isCanGo(neighborState)) {
                    continue;
                }
                int updist = 0;
                if (isBlocked(neighborState) && !isOnGround(neighborState) && facingidx == 3) {
                    updist++;
                    neighborCoordinate.y -= 1;
                    neighbor = &GetNode(neighborCoordinate );
                    neighborState = request.octNodeWorld.getOctNode(neighborCoordinate).data;


                    if (isBlocked(neighborState) && !isOnGround(neighborState))
                        continue;
                }
                if (neighborCoordinate.y < minY - 5) continue;
                if (neighborCoordinate.y >= maxY + 5) continue;

                if (isBlocked(neighborState) && !isOnGround(neighborState) && facingidx != 2 && facingidx != 3)
                    continue;

                bool superboomthingy = (originNodeState == COLLISION_STATE_SUPERBOOMABLE_AIR || originNodeState == COLLISION_STATE_SUPERBOOMABLE_GROUND) &&
                                          (neighborState != COLLISION_STATE_SUPERBOOMABLE_AIR && neighborState != COLLISION_STATE_SUPERBOOMABLE_GROUND);

                float gScore = n.gScore+ (superboomthingy ? 10 : isOnGround(neighborState) || facingidx == 2 ? 1 : 2 * (updist + 1));
                if (gScore < neighbor->gScore) {
                    neighbor->parent = coord;
                    if (isBlocked(neighborState))
                        neighbor->stonkLen = (n.stonkLen + 1 + updist);
                    else
                        neighbor->stonkLen = 0;

                    if (superboomthingy)
                        neighbor->type = ConnectionType_SUPERBOOM;
                    else if (isBlocked(neighborState))
                        neighbor->type = ConnectionType_STONK_EXIT;
                    else
                        neighbor->type = ConnectionType_WALK;
                    neighbor->gScore= gScore;

                    pq.emplace(neighbor->gScore, neighborCoordinate);
                }
            }
        }
    }
}


int TRANSFORM_MATRICES[24][9] = {
        // rotated vers
        {1,0,0, 0,1,0,0,0,1},
        {0,1,0, 0,0,1,1,0,0},
        {0,0,1, 1,0,0,0,1,0}, // you might ask, "but shouldn't there be 6 rotated versions?" well xyz and yxz is same thing, because prism is symmetric. So divide by two.

        // flipping!
        {-1,0,0, 0,1,0,0,0,1},
        {0,-1,0, 0,0,1,1,0,0},
        {0,0,-1, 1,0,0,0,1,0},

        {1,0,0, 0,-1,0,0,0,1},
        {0,1,0, 0,0,-1,1,0,0},
        {0,0,1, -1,0,0,0,1,0},

        {-1,0,0, 0,-1,0,0,0,1},
        {0,-1,0, 0,0,-1,1,0,0},
        {0,0,-1, -1,0,0,0,1,0},

        {1,0,0, 0,1,0,0,0,-1},
        {0,1,0, 0,0,1,-1,0,0},
        {0,0,1, 1,0,0,0,-1,0},

        {-1,0,0, 0,1,0,0,0,-1},
        {0,-1,0, 0,0,1,-1,0,0},
        {0,0,-1, 1,0,0,0,-1,0},

        {1,0,0, 0,-1,0,0,0,-1},
        {0,1,0, 0,0,-1,-1,0,0},
        {0,0,1, -1,0,0,0,-1,0},
        {-1,0,0, 0,-1,0,0,0,-1},
        {0,-1,0, 0,0,-1,-1,0,0},
        {0,0,-1, -1,0,0,0,-1,0},

};

void Pathfinder::ShadowCast(int centerX, int centerY, int centerZ, int startZ, float startSlopeX,
                            float endSlopeX, float startSlopeY, float endSlopeY, int  radius,
                            float xOffset, float yOffset, float zOffset, int trMatrix11,
                            int  trMatrix21, int  trMatrix31, int  trMatrix12, int  trMatrix22,
                            int  trMatrix32, int  trMatrix13, int  trMatrix23, int  trMatrix33,
                            vector<Coordinate> &result) {
    if (startZ > radius) return;
    shadowcasts++;
    // boom. radius is manhatten radius. lol.
    float realZ = startZ - zOffset;

    int startY =  max(0, (int) floor(startSlopeY * (realZ - 0.5) - 0.5 + yOffset)) - 1;
    int endY = (int) ceil(endSlopeY * (realZ + 0.5) - 0.5 + yOffset) + 1;
    int startX = max(0, (int) floor(startSlopeX * (realZ - 0.5) - 0.5 + xOffset)) - 1;
    int endX = (int) ceil(endSlopeX * (realZ + 0.5) - 0.5 + xOffset) + 1;
    int yLen = endY - startY + 1;
    int xLen = endX - startX + 1;
    bool blockMap[yLen][xLen];
    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            int trX = centerX + x * trMatrix11 + y * trMatrix21 + startZ * trMatrix31;
            int trY = centerY + x * trMatrix12 + y * trMatrix22 + startZ * trMatrix32;
            int trZ = centerZ + x * trMatrix13 + y * trMatrix23 + startZ * trMatrix33;

            bool localBlocked = request.blockWorld.getBlock(trX, trY, trZ).id != 0 && startZ != 0;
            if (endSlopeX == 1 && x +1 == endX && blockMap[y - startY][x-1 - startX]) localBlocked = true;
            if (endSlopeY == 1 && y +1 == endY && blockMap[y-1 - startY][x - startX]) localBlocked = true;

            blockMap[y - startY][x - startX] = localBlocked;

        }
    }
    for (int y = startY * 2 + 1; y < endY * 2; y ++) {
        float currentSlopeY = ((y-yOffset) / 2.0f) / (realZ-0.5f);
        float currentSlopeYP = ((y-yOffset) / 2.0f) / (realZ);
        for (int x = startX * 2 + 1; x < endX * 2; x++) {
            float currentSlopeX = ((x-xOffset) / 2.0f) / (realZ-0.5f) ;
            float currentSlopeXP = ((x-xOffset) / 2.0f) / (realZ);

            bool localBlocked = blockMap[y/2 - startY][x/2 - startX];
            if (x%2 != 0) localBlocked &= blockMap[y/2 - startY][x/2 - startX+1];
            if (y%2 != 0) localBlocked &= blockMap[y/2 - startY +1][x/2 - startX];
            if (x%2 != 0 && y%2 != 0) localBlocked &= blockMap[y/2 - startY + 1][x/2 - startX+1];
            if (localBlocked) continue;

            if (!(currentSlopeY < startSlopeY || currentSlopeY > endSlopeY || currentSlopeX < startSlopeX || currentSlopeX > endSlopeX)) [[likely]] {
                int trX = centerX * 2 + 1+ (x) * trMatrix11 + (y) * trMatrix21 + (startZ*2-1 ) * trMatrix31;
                int trY = centerY * 2 + 1+ (x) * trMatrix12 + (y) * trMatrix22 + (startZ*2 -1) * trMatrix32;
                int trZ = centerZ * 2 + 1+ (x) * trMatrix13 + (y) * trMatrix23 + (startZ*2-1) * trMatrix33;
                result.push_back({trX, trY, trZ});
            }
            if (!(currentSlopeYP < startSlopeY || currentSlopeYP > endSlopeY || currentSlopeXP < startSlopeX || currentSlopeXP > endSlopeX)) [[likely]]  {
                int trX = centerX * 2 + 1+ (x) * trMatrix11 + (y) * trMatrix21 + (startZ*2 ) * trMatrix31;
                int trY = centerY * 2 + 1+ (x) * trMatrix12 + (y) * trMatrix22 + (startZ*2 ) * trMatrix32;
                int trZ = centerZ * 2 + 1+ (x) * trMatrix13 + (y) * trMatrix23 + (startZ*2 ) * trMatrix33;
                result.push_back({trX, trY, trZ});
            }
        }
    }
    bool yEdges[yLen];
    bool xEdges[xLen];
    for (int y = 0; y < yLen ; y++) {
        for (int x = 0; x < xLen; x++) {
            if (y < yLen -1 && blockMap[y][x] != blockMap[y+1][x])
                yEdges[y] = true;
            else
                yEdges[y] = false;
            if (x < xLen - 1 && blockMap[y][x] != blockMap[y][x+1])
                xEdges[x] = true;
            else
                xEdges[x] = false;
        }
    }
    yEdges[yLen - 1] = true;
    xEdges[xLen - 1] = true;


    int prevY = -1;
    float leeway = request.settings.etherwarpLeeway;
    for (int y = 0; y < yLen; y++) {
        if (!yEdges[y]) [[likely]] continue;
        int prevX = -1;
        for (int x = 0; x < xLen; x++) {
            if (!xEdges[x]) [[likely]] continue;
            if (!blockMap[y][x]) {
                bool yGood = prevY != -1 && !blockMap[prevY][x];
                bool xGood = prevX != -1 && !blockMap[y][prevX];
                bool diagonalGood = prevY != -1 && prevX != -1 && !blockMap[prevY][prevX];


                if (diagonalGood && yGood && xGood) {
                    float startSlopeYY = max(startSlopeY, (prevY + startY - yOffset + 0.5f - leeway) / (realZ + 0.5f));
                    float endSlopeYY = min(endSlopeY, (y + startY - yOffset + 0.5f - leeway) / (realZ + 0.5f));
                    float startSlopeXX  = max(startSlopeX, (prevX  + startX - xOffset+ 0.5f - leeway) / (realZ + 0.5f));
                    float endSlopeXX = min(endSlopeX, (x + startX - xOffset  + 0.5f - leeway) / (realZ + 0.5f));
                    if (startSlopeYY < endSlopeYY && startSlopeXX < endSlopeXX) {
                        [[likely]]
                        ShadowCast(centerX, centerY, centerZ, startZ + 1, startSlopeXX, endSlopeXX,
                                   startSlopeYY ,endSlopeYY , radius,
                                   xOffset, yOffset, zOffset, trMatrix11, trMatrix21, trMatrix31, trMatrix12, trMatrix22, trMatrix32, trMatrix13, trMatrix23, trMatrix33, result);
                    }
                } else if ((xGood && !yGood) || (yGood && !xGood)) {
                    float startSlopeYY = yGood ? max(startSlopeY, (prevY + startY - yOffset +0.5f - leeway) / (realZ + 0.5f)) : max(startSlopeY, (prevY + startY - yOffset +0.5f + leeway) / (realZ - 0.5f));
                    float endSlopeYY = min(endSlopeY, (y + startY - yOffset + 0.5f - leeway) / (realZ + 0.5f));
                    float startSlopeXX  = xGood ? max(startSlopeX, (prevX  + startX - xOffset+0.5f - leeway) / (realZ + 0.5f)) : max(startSlopeX, (prevX  + startX - xOffset+ 0.5f + leeway) / (realZ - 0.5f));
                    float endSlopeXX = min(endSlopeX, (x + startX - xOffset  + 0.5f - leeway) / (realZ + 0.5f));
                    if (startSlopeYY < endSlopeYY && startSlopeXX < endSlopeXX) {
                        [[likely]]
                        ShadowCast(centerX, centerY, centerZ, startZ + 1, startSlopeXX, endSlopeXX,
                                   startSlopeYY ,endSlopeYY , radius,
                                   xOffset, yOffset, zOffset, trMatrix11, trMatrix21, trMatrix31, trMatrix12, trMatrix22, trMatrix32, trMatrix13, trMatrix23, trMatrix33, result);
                    }
                } else if (!diagonalGood && yGood && xGood) {
                    {
                        float startSlopeYY = max(startSlopeY, (prevY + startY - yOffset +0.5f + leeway) / (realZ - 0.5f));
                        float endSlopeYY = min(endSlopeY, (y + startY - yOffset + 0.5f - leeway) / (realZ + 0.5f));
                        float startSlopeXX = max(startSlopeX, (prevX + startX - xOffset + 0.5f - leeway) / (realZ + 0.5f));
                        float endSlopeXX = min(endSlopeX, (x + startX - xOffset + 0.5f - leeway) / (realZ + 0.5f));
                        if (startSlopeYY < endSlopeYY && startSlopeXX < endSlopeXX) {
                            [[likely]]
                            ShadowCast(centerX, centerY, centerZ, startZ + 1, startSlopeXX, endSlopeXX,
                                       startSlopeYY, endSlopeYY, radius,
                                       xOffset, yOffset, zOffset,  trMatrix11, trMatrix21, trMatrix31, trMatrix12, trMatrix22, trMatrix32, trMatrix13, trMatrix23, trMatrix33, result);
                        }
                    }
                    {
                        float startSlopeYY = max(startSlopeY, (prevY + startY - yOffset +0.5f - leeway) / (realZ + 0.5f));
                        float endSlopeYY = min(endSlopeY, (prevY + startY - yOffset + 0.5f + leeway) / (realZ - 0.5f));
                        float startSlopeXX  =  max(startSlopeX, (prevX  + startX - xOffset+0.5f + leeway) / (realZ - 0.5f));
                        float endSlopeXX = min(endSlopeX, (x + startX - xOffset + 0.5f - leeway) / (realZ + 0.5f));
                        if (startSlopeYY < endSlopeYY && startSlopeXX < endSlopeXX) {
                            [[likely]]
                            ShadowCast(centerX, centerY, centerZ, startZ + 1, startSlopeXX, endSlopeXX,
                                       startSlopeYY ,endSlopeYY , radius,
                                       xOffset, yOffset, zOffset,  trMatrix11, trMatrix21, trMatrix31, trMatrix12, trMatrix22, trMatrix32, trMatrix13, trMatrix23, trMatrix33, result);
                        }
                    }
                } else {
                    float startSlopeYY = max(startSlopeY, (prevY + startY - yOffset + 0.5f + leeway) / (realZ - 0.5f));
                    float endSlopeYY = min(endSlopeY, (y + startY - yOffset + 0.5f - leeway) / (realZ + 0.5f));
                    float startSlopeXX  = max(startSlopeX, (prevX  + startX - xOffset+ 0.5f + leeway) / (realZ - 0.5f));
                    float endSlopeXX = min(endSlopeX, (x + startX - xOffset + 0.5f - leeway) / (realZ + 0.5f));
                    if (startSlopeYY < endSlopeYY && startSlopeXX < endSlopeXX) {
                        [[likely]]
                        ShadowCast(centerX, centerY, centerZ, startZ + 1, startSlopeXX, endSlopeXX,
                                   startSlopeYY ,endSlopeYY , radius,
                                   xOffset, yOffset, zOffset,  trMatrix11, trMatrix21, trMatrix31, trMatrix12, trMatrix22, trMatrix32, trMatrix13, trMatrix23, trMatrix33, result);
                    }
                    // normal case
                }
            }
            prevX = x;
        }
        prevY = y;
    }
}

void Pathfinder::RealShadowCast(std::vector<Coordinate>& result, Coordinate start, int radius) {
    float offset = request.settings.etherwarpOffset;
        for (auto & i : TRANSFORM_MATRICES) {
            if (request.blockWorld.getBlock(start.x + 1 * i[0], start.y + 1 * i[3], start.z + 1 * i[6]).id == 0)
            ShadowCast(start.x, start.y, start.z, 1,0, 1, 0, 1, radius,
                       offset, 0, 0,
                       i[0] ,i[1],i[2],
                       i[3], i[4], i[5],
                       i[6],i[7], i[8], result
            );
            if (request.blockWorld.getBlock(start.x + 1 * i[1], start.y + 1 * i[4], start.z + 1 * i[7]).id == 0)
            ShadowCast(start.x, start.y, start.z, 1,0, 1, 0, 1, radius,
                       0, offset, 0,
                       i[0] ,i[1],i[2],
                       i[3], i[4], i[5],
                       i[6],i[7], i[8], result
            );
            if (request.blockWorld.getBlock(start.x + 1 * i[2], start.y + 1 * i[5], start.z + 1 * i[8]).id == 0)
            ShadowCast(start.x, start.y, start.z, 1,0, 1, 0, 1, radius,
                       0, 0, offset,
                       i[0] ,i[1],i[2],
                       i[3], i[4], i[5],
                       i[6],i[7], i[8], result
            );
        }
}

Pathfinder::Pathfinder(PathfindRequest& req): request(req) {
    int state = 0;
    this->minY = -1;
    this-> maxY = -1;
    for (int i = 0; i < 256; i++) {
        Block b = req.blockWorld.getBlock(8, i, 8);
        if (b.id != 0 && state == 0) {
            state = 1;
            this->minY = i * 2;
        } else if (b.id == 0 && state == 1) {
            state = 2;
            this->maxY = i * 2;
        } else if (b.id != 0 && state == 2) {
            state = 1;
        }
    }
    if (minY == maxY) {
        state = 0;
        for (int i = 0; i < 256; i++) {
            Block b = req.blockWorld.getBlock(req.blockWorld.xLen - 8,i,req.blockWorld.zLen - 8);
            if (b.id != 0 && state == 0){
                state = 1;
                this->minY = i * 2;
            } else if (b.id == 0 && state == 1) {
                state = 2;
                this->maxY = i*2;
            } else if (b.id != 0 && state == 2) {
                state = 1;
            }
        }
    }
    nodes = vector<vector<vector<PathfindNode>>> (request.octNodeWorld.xLen + 10,
                                                  vector<vector<PathfindNode> >(request.octNodeWorld.zLen + 10,
                                                                                vector<PathfindNode>((maxY - minY) + 10, PathfindNode{})));
    shadowcasts = 0;
    std::cout << "Min " << minY << " Max " <<maxY << std::endl;
}
