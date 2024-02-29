//
// Created by syeyoung on 2024/02/27.
//
#include <iostream>
#include <fstream>
#include <filesystem>
#include "PathfindRequest.h"
#include "Pathfinder.h"
#include "PathfindResult.h"

template <
        class result_t   = std::chrono::milliseconds,
        class clock_t    = std::chrono::steady_clock,
        class duration_t = std::chrono::milliseconds
>
auto since(std::chrono::time_point<clock_t, duration_t> const& start)
{
    return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Correct usage: " << argv[0] << " [input.pfreq] [output.file]" << std::endl;
        exit(1);
    }
    std::string input = argv[1];
    std::string output = argv[2];
    std::cout << "Processing: " << input << std::endl;

    if (!std::filesystem::exists(input)) {
        std::cerr << "Input File not found: " << input << std::endl;
        exit(-1);
    }

    if (std::filesystem::exists(output)) {
        std::cerr << "Output File found: " << output << std::endl;
        exit(-1);
    }


    PathfindRequest request;
    std::ifstream input_file(input, std::ios_base::binary);
    try {
        request.ReadRequest(input_file);
    } catch (std::string &ex) {
        std::cerr << ex << std::endl;
        exit(-1);
    }

    std::cout << "ID: " << request.uuid << " NAME: " << request.name << std::endl;
    std::cout << "PF ID: " << request.id << std::endl;
    std::cout << "Target Cnt: "<< request.target.size() << " World Size: " << request.blockWorld.xLen * request.blockWorld.yLen * request.blockWorld.zLen << std::endl;
    std::cout << "Etherwarp settings: " << request.settings.etherwarpRadius << " leeway " << request.settings.etherwarpLeeway << " offset "<<request.settings.etherwarpOffset << std::endl;


    auto start = std::chrono::steady_clock::now();

    Pathfinder pathfinder(request);
    pathfinder.Populate();

    std::cout << "Elapsed(ms)=" << since(start).count() << std::endl;


    long cnt = 0;
    int x = -6;
    for (auto &item: pathfinder.nodes) {
        x++;
        int z = -6;
        for (auto &item2: item) {
            z++;
            int y = pathfinder.minY-6;
            for (auto &item3: item2) {
                y++;
                if (item3.gScore < 9999999) {
                    cnt++;
                }

//                if (item3.type != ConnectionType_UNPOPULATED) {
//                    // check cycle.
//
//                    PathfindNode *slowPtr = &item3;
//                    PathfindNode *fastPtr = &item3;
//                    do {
//                        if (slowPtr->type != ConnectionType_TARGET)
//                            slowPtr = &pathfinder.GetNode(slowPtr ->parent);
//                        if (fastPtr->type != ConnectionType_TARGET)
//                            fastPtr = &pathfinder.GetNode(fastPtr -> parent);
//                        if (fastPtr->type != ConnectionType_TARGET)
//                            fastPtr = &pathfinder.GetNode(fastPtr -> parent);
//                    } while (fastPtr != slowPtr && fastPtr->type != ConnectionType_TARGET);
//                    if (fastPtr -> type != ConnectionType_TARGET) {
//                        std::cout << "Cycle detected at "<<x<<" "<<y<<" "<<z << std::endl;
//                    }
//                }

// 634300
// 19513203
// 2856
// 1263476575
// 1263477197
// 62780341
// 1268187487

            }
        }
    }
    std::cout << "Open Nodes: " << cnt << std::endl << std::endl;
    std::cout << "Shadow cast called: " << pathfinder.shadowcasts << "times" << std::endl;

    std::ofstream outfile_1(output, std::ios_base::binary);
    PathfindResult result;
    result.Init(pathfinder);
    result.WriteTo(outfile_1);


    return 0;
}
