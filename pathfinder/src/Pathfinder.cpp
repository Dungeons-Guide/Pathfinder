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
                this->shadowCaster.RealShadowCast([&](int x, int y, int z) {
                    if (_distSq(x - coord.x, y - coord.y - 1, z - coord.z) > request.settings.etherwarpRadius * request.settings.etherwarpRadius * 4) return ;
                    if (x < 2) return;
                    if (y -3 < minY - 5) return;
                    if (z < 2) return;
                    if (x >= request.octNodeWorld.xLen ) return;
                    if (y -3 >= maxY + 5) return;
                    if (z  >= request.octNodeWorld.zLen ) return;

                    y -= 3;

                    PathfindNode *neighbor = &GetNode(x,y,z);
                    auto neighborState = request.octNodeWorld.getOctNode(x,y,z).data;
                    if (!isOnGround(neighborState)) return;

                    float gScore = n.gScore+ 20; // don't etherwarp unless it saves like 10 blocks
                    if (gScore < neighbor->gScore) {
                        neighbor->parent = coord;
                        neighbor->stonkLen = 0;
                        neighbor->type = ConnectionType_ETHERWARP;
                        neighbor->gScore= gScore;

                        pq.emplace(neighbor->gScore, Coordinate{x,y,z});
                    }
                    }, start, request.settings.etherwarpRadius);
//                for (Coordinate& target : shadowcastRes) {
//
//                }
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



Pathfinder::Pathfinder(PathfindRequest& req): shadowCaster(req), request(req) {
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
}
