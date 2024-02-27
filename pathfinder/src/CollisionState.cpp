//
// Created by syeyoung on 2024/02/26.
//
#include "../include/CollisionState.h"

bool isCanGo(CollisionState collisionState) {
    return collisionState < COLLISION_STATE_BLOCKED;
}
bool isClip(CollisionState collisionState) {
    return collisionState >= COLLISION_STATE_STAIR;
}
bool isBlocked(CollisionState collisionState) {
    return collisionState >= COLLISION_STATE_STONKING;
}
bool isOnGround(CollisionState collisionState) {
    return collisionState == COLLISION_STATE_ONGROUND || collisionState == COLLISION_STATE_SUPERBOOMABLE_GROUND || collisionState == COLLISION_STATE_STAIR || collisionState == COLLISION_STATE_ENDERCHEST || collisionState == COLLISION_STATE_STONKING || collisionState == COLLISION_STATE_BLOCKED;
}