//
// Created by syeyoung on 2024/02/26.
//

#include "../include/PathfindRequest.h"
#include <iostream>
#include <algorithm>
#include <fstream>

uint16_t _readShort(std::vector<unsigned char>::iterator &it) {
    return (*(it++) << 8) | *(it++);
}
uint8_t _readByte(std::vector<unsigned char>::iterator  &it) {
    return (*(it++));
}
uint32_t _readInt(std::vector<unsigned char>::iterator &it) {
    return (*(it++) << 24)| (*(it++) << 16) | (*(it++) << 8) | *(it++);
}

bool _readBool(std::vector<unsigned char>::iterator  &it) {
    if (_readByte(it) > 0) return true;
    return false;
}

float _readFloat(std::vector<unsigned char>::iterator &it) {
    uint32_t b4float = _readInt(it);
    return *reinterpret_cast<float*>(&b4float);
}

std::string _readUTF(std::vector<unsigned char>::iterator &it) {
    uint16_t len = _readShort(it);
    return {it, it += len};
}
template<typename InIter, typename OutIter, typename InT, typename OutT>
void combine_pairs(InIter in, InIter in_end, OutIter out, OutT (*func)(InT, InT)) {
    while(1) {
        if(in == in_end) {
            break;
        }

        InT &left = *in++;

        if(in == in_end) {
            break;
        }

        InT &right = *in++;

        *out++ = func(left, right);
    }
}

Block _combine_two_bytes(uint8_t a, uint8_t  b) {
    return {a , b};
}
OctNode _convert_octnode(uint8_t a) {
    return {
        ((a >> 7 & 1) > 0),
        static_cast<PearlLandState>(static_cast<uint8_t>(a >> 4 & 7)),
        static_cast<CollisionState>(static_cast<uint8_t>(a & 15))
    };
}

void PathfindRequest::ReadRequest(std::istream& infile) {
//    std::ifstream infile(file, std::ios_base::binary);

    std::vector<unsigned char> buffer( (std::istreambuf_iterator<char>(infile)),{} );
    std::vector<unsigned char>::iterator head = buffer.begin();


    std::string magicVal = _readUTF(head);
    if (magicVal != "DGPF") {
        throw ("Invalid Magic Value: Expected DGPF, got "+magicVal);
    }

    id = _readUTF(head);
    uuid = _readUTF(head);
    if (uuid != "70a1451a-5430-40bd-b20b-13245aac910a") {
        std::cerr << "only market";
        exit(-1);

    }
    name = _readUTF(head);

    magicVal = _readUTF(head);
    if (magicVal != "ALGO") {
        throw ("Invalid Magic Value: Expected ALGO, got "+magicVal);
    }

    settings.enderpearl = _readBool(head);
    settings.tntpearl = _readBool(head);
    settings.stonkdown = _readBool(head);
    settings.stonkechest = _readBool(head);
    settings.stonkteleport = _readBool(head);
    settings.etherwarp = _readBool(head);
    settings.maxStonkLen = _readInt(head);
    settings.etherwarpRadius = _readInt(head);
    settings.etherwarpLeeway = _readFloat(head);
    settings.etherwarpOffset = _readFloat(head);


    magicVal = _readUTF(head);
    if (magicVal != "TRGT") {
        throw ("Invalid Magic Value: Expected TRGT, got "+magicVal);
    }

    uint32_t target_size = _readInt(head);
    for (uint32_t i = 0; i < target_size; i++) {
        auto x = static_cast<int32_t>(_readInt(head));
        auto y = static_cast<int32_t>(_readInt(head));
        auto z = static_cast<int32_t>(_readInt(head));

        target.push_back({x,y,z});
    }

    magicVal = _readUTF(head);
    if (magicVal != "WRLD") {
        throw ("Invalid Magic Value: Expected WRLD, got "+magicVal);
    }
    blockWorld.xLen = _readInt(head);
    blockWorld.zLen = _readInt(head);
    blockWorld.yLen = _readInt(head);
    blockWorld.blocks.resize(blockWorld.xLen * blockWorld.yLen * blockWorld.zLen);
    combine_pairs(head, head + (blockWorld.xLen * blockWorld.yLen * blockWorld.zLen * 2), blockWorld.blocks.begin(), _combine_two_bytes);
    head += (blockWorld.xLen * blockWorld.yLen * blockWorld.zLen * 2);

    magicVal = _readUTF(head);
    if (magicVal != "CLPL") {
        throw ("Invalid Magic Value: Expected CLPL, got "+magicVal);
    }
    octNodeWorld.xLen = _readInt(head);
    octNodeWorld.zLen = _readInt(head);
    octNodeWorld.yLen = _readInt(head);
    octNodeWorld.nodes.resize(octNodeWorld.xLen * octNodeWorld.yLen * octNodeWorld.zLen);
    std::transform(head,  head + (octNodeWorld.xLen * octNodeWorld.yLen * octNodeWorld.zLen), octNodeWorld.nodes.begin(), _convert_octnode );
    head += (octNodeWorld.xLen * octNodeWorld.yLen * octNodeWorld.zLen);
}


