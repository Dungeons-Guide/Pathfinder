//
// Created by syeyoung on 2024/02/26.
//

#include "../include/PathfindResult.h"
#include <fstream>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zlib.hpp>


void PathfindResult::Init(Pathfinder &pathfinder) {
    this->id = pathfinder.request.id;
    this->name = pathfinder.request.name;
    this->uuid = pathfinder.request.uuid;
    this->target = pathfinder.request.target;
    this->settings = pathfinder.request.settings;
    this->resultXStart = 0;
    this->resultYStart = pathfinder.minY;
    this->resultZStart = 0;
    this->resultXLen = pathfinder.request.octNodeWorld.xLen;
    this->resultYLen = pathfinder.maxY - pathfinder.minY;
    this->resultZLen = pathfinder.request.octNodeWorld.zLen;
    this->results.resize(resultXLen * resultYLen * resultZLen);
    for (int y = 0; y < resultYLen; y++) {
        for (int z = 0; z < resultZLen; z++) {
            for (int x = 0; x < resultXLen; x++) {
                auto node = pathfinder.GetNode(x,y + pathfinder.minY,z);
                if (node.parent.x < resultXStart || node.parent.x >= resultXStart + resultXLen ||
                    node.parent.y < resultYStart || node.parent.y >= resultYStart + resultYLen ||
                    node.parent.z < resultZStart || node.parent.z >= resultZStart + resultZLen) {
                    this->results[y * resultXLen * resultZLen + z * resultXLen + x] = {
                            ResultCoordinate {
                                    0,0,0
                            },
                            std::numeric_limits<float>::max(),
                            ConnectionType_UNPOPULATED
                    };
                } else {
                    this->results[y * resultXLen * resultZLen + z * resultXLen + x] = {
                            ResultCoordinate {
                                    (uint8_t) (node.parent.x - resultXStart),
                                    (uint8_t) (node.parent.y - resultYStart),
                                    (uint8_t) (node.parent.z - resultZStart)
                            },
                            (float) node.gScore,
                            node.type
                    };
                }

            }
        }
    }
}

void _writeByte(std::ostream& stream, uint8_t c) {
    char cs = (char) c;
    stream.write(&cs, 1);
}
void _writeShort(std::ostream& stream, uint16_t c) {
    char arr[2] = {
            static_cast<char>((c >> 8) & 0xFF),
            static_cast<char>((c) & 0xFF)
    };
    stream.write(arr, 2);
}
void _writeInt(std::ostream& stream, uint32_t c) {
    char arr[4] = {
            static_cast<char>((c >> 24) & 0xFF),
            static_cast<char>((c >> 16) & 0xFF),
            static_cast<char>((c >> 8) & 0xFF),
            static_cast<char>((c) & 0xFF)
    };
    stream.write(arr, 4);
}
void _writeUTF(std::ostream & stream, std::string value) {
    _writeShort(stream, value.size());
    stream.write(value.c_str(), value.size());
}
void _writeBool(std::ostream & stream, bool wat) {
    _writeByte(stream, wat ? 1 : 0);
}
void _writeFloat(std::ostream & stream, float f) {
    char const * p = reinterpret_cast<char const *>(&f);
    stream.write(p, 4);
}


void PathfindResult::WriteTo(std::ostream& outfile_1) {
//


    _writeUTF(outfile_1, "RDGPF");
    _writeUTF(outfile_1, id);
    _writeUTF(outfile_1, uuid);
    _writeUTF(outfile_1, name);
    _writeUTF(outfile_1, "ALGO");

    _writeBool(outfile_1, settings.enderpearl);
    _writeBool(outfile_1, settings.tntpearl);
    _writeBool(outfile_1, settings.stonkdown);
    _writeBool(outfile_1, settings.stonkechest);
    _writeBool(outfile_1, settings.stonkteleport);
    _writeBool(outfile_1, settings.etherwarp);
    _writeInt(outfile_1, settings.maxStonkLen);
    _writeUTF(outfile_1, "TRGT");
    _writeInt(outfile_1, target.size());
    for (const auto &item: target) {
        _writeInt(outfile_1, item.x);
        _writeInt(outfile_1, item.y);
        _writeInt(outfile_1, item.z);
    }
    _writeUTF(outfile_1, "NODE");

    boost::iostreams::filtering_ostream outfile;
    outfile.push(boost::iostreams::zlib_compressor());
    outfile.push(outfile_1);
    _writeShort(outfile, resultXStart);
    _writeShort(outfile, resultYStart);
    _writeShort(outfile, resultZStart);
    _writeShort(outfile, resultXLen);
    _writeShort(outfile, resultYLen);
    _writeShort(outfile, resultZLen);
    for (const auto &item: results) {
        _writeByte(outfile, item.parent.x);
        _writeByte(outfile, item.parent.y);
        _writeByte(outfile, item.parent.z);
        _writeByte(outfile, item.type);
        _writeFloat(outfile, item.gScore); // 8 byte per block. umm... lol.
    }
    _writeUTF(outfile, "EOF");
}
