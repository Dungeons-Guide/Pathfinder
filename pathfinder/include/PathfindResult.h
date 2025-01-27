//
// Created by syeyoung on 2024/02/26.
//

#include "cstdint"
#include <vector>
#include "Pathfinder.h"

#ifndef PATHFINDER2_PATHFINDRESULT_H
#define PATHFINDER2_PATHFINDRESULT_H

struct ResultCoordinate {
    uint8_t x;
    uint8_t y;
    uint8_t z;
};

struct ResultNode {
    ResultCoordinate parent = {0,0,0};
    float gScore;
    ConnectionType type = ConnectionType_UNPOPULATED;
};

class PathfindResult {
public:
    std::string hash;
    std::string id;
    std::string uuid;
    std::string name;
    std::string roomstate;
    AlgorithmSettings* settings;
    std::vector<Coordinate> target;
    std::vector<ResultNode> results;
    uint16_t resultXLen;
    uint16_t resultYLen;
    uint16_t resultZLen;
    uint16_t resultXStart;
    uint16_t resultYStart;
    uint16_t resultZStart;

    void Init(Pathfinder& pathfinder);
    void WriteTo(std::ostream& file, std::string source);


};

#endif //PATHFINDER2_PATHFINDRESULT_H
