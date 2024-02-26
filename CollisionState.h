//
// Created by syeyoung on 2024/02/26.
//

#ifndef PATHFINDER2_COLLISIONSTATE_H
#define PATHFINDER2_COLLISIONSTATE_H

#include <cstdint>

enum CollisionState : uint8_t {
    COLLISION_STATE_ONAIR = 0,
    COLLISION_STATE_ONGROUND = 1,
    COLLISION_STATE_SUPERBOOMABLE_GROUND = 2,
    COLLISION_STATE_SUPERBOOMABLE_AIR = 3,
    COLLISION_STATE_STAIR = 4,
    COLLISION_STATE_ENDERCHEST = 5,
    COLLISION_STATE_STONKING = 6,
    COLLISION_STATE_STONKING_AIR = 7,
    COLLISION_STATE_BLOCKED = 8,
    COLLISION_STATE_BLOCKED_AIR = 9
};
bool isCanGo(CollisionState collisionState);
bool isClip(CollisionState collisionState);
bool isBlocked(CollisionState collisionState);
bool isOnGround(CollisionState collisionState);
#endif //PATHFINDER2_COLLISIONSTATE_H
