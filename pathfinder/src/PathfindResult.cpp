//
// Created by syeyoung on 2024/02/26.
//

#include "../include/PathfindResult.h"
#include <fstream>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <random>
#include <sstream>
#include <iomanip>

void PathfindResult::Init(Pathfinder &pathfinder) {
    this->id = pathfinder.request.id;
    this->hash = pathfinder.request.idhash;
    this->name = pathfinder.request.roomname;
    this->roomstate = pathfinder.request.roomstate;
    this->uuid = pathfinder.request.roomuuid;
    this->target = pathfinder.request.target;
    this->settings = &pathfinder.request.settings;
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
                if (node.type == ConnectionType_TARGET) {
                    this->results[y * resultXLen * resultZLen + z * resultXLen + x] = {
                            ResultCoordinate {
                                    0,0,0
                            },
                            (float) node.gScore,
                            node.type
                    };
                } else if ((node.parent.x < resultXStart || node.parent.x >= resultXStart + resultXLen ||
                    node.parent.y < resultYStart || node.parent.y >= resultYStart + resultYLen ||
                    node.parent.z < resultZStart || node.parent.z >= resultZStart + resultZLen)) {
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
void _writeBytes(std::ostream & stream, std::string value) {
    stream.write(value.c_str(), value.size());
}
void _writeBool(std::ostream & stream, bool wat) {
    _writeByte(stream, wat ? 1 : 0);
}
void _writeFloat(std::ostream & stream, float f) {
    char const * p = reinterpret_cast<char const *>(&f);
    stream.write(p, 4);
}


std::string generateRandom128BitHex() {
    std::random_device rd;
    std::mt19937_64 gen(rd()); // Use 64-bit Mersenne Twister for random numbers
    std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

    std::ostringstream oss;
    for (int i = 0; i < 2; ++i) { // 128 bits = 2 * 64-bit numbers
        uint64_t randomNum = dist(gen);
        oss << std::hex << std::setw(16) << std::setfill('0') << randomNum;
    }

    return oss.str();
}


void PathfindResult::WriteTo(std::ostream& outfile_1, std::string source) {
//

    _writeBytes(outfile_1, "DGPFRES2");
    _writeInt(outfile_1, 1);
    _writeUTF(outfile_1, generateRandom128BitHex());
    _writeUTF(outfile_1, hash);
    _writeUTF(outfile_1, id);
    _writeUTF(outfile_1, uuid);
    _writeUTF(outfile_1, roomstate);
    _writeUTF(outfile_1, source);
    _writeBytes(outfile_1, "ALGO");

    settings->WriteAlgorithmSettings(outfile_1);


    _writeBytes(outfile_1, "TRGT");
    _writeInt(outfile_1, target.size());
    for (const auto &item: target) {
        _writeInt(outfile_1, item.x);
        _writeInt(outfile_1, item.y);
        _writeInt(outfile_1, item.z);
    }
    _writeBytes(outfile_1, "NODE");
    _writeByte(outfile_1, 1);

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
    _writeBytes(outfile, "EOF");
}
