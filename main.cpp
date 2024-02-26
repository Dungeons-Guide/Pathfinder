#include <iostream>
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
    try {
        request.ReadRequest(input);
    } catch (std::string &ex) {
        std::cerr << ex << std::endl;
        exit(-1);
    }

    std::cout << "ID: " << request.uuid << " NAME: " << request.name << std::endl;
    std::cout << "PF ID: " << request.id << std::endl;
    std::cout << "Target Cnt: "<< request.target.size() << std::endl;
    std::cout << "World Size: " << request.blockWorld.xLen * request.blockWorld.yLen * request.blockWorld.zLen << std::endl;

    auto start = std::chrono::steady_clock::now();

    Pathfinder pathfinder(request);
    pathfinder.Populate();

    std::cout << "Elapsed(ms)=" << since(start).count() << std::endl;

    long cnt = 0;
    for (const auto &item: pathfinder.nodes) {
        for (const auto &item2: item) {
            for (const auto &item3: item2) {
                if (item3.gScore < 9999999) {
                    cnt++;
                }
            }
        }
    }
    std::cout << "Open Nodes: " << cnt << std::endl << std::endl;

    PathfindResult result;
    result.Init(pathfinder);
    result.WriteTo(output);


    return 0;
}
