//
// Created by syeyoung on 2024/02/26.
//

#include "../include/PathfindRequest.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <io/stream_reader.h>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zlib.hpp>

uint16_t _readShort(std::istream& in) {
    uint8_t buffer[2];
    in.read(reinterpret_cast<char *>(buffer), 2);
    return buffer[0] << 8 | buffer[1];
}
uint8_t _readByte(std::istream& in) {
    uint8_t buffer;
    in.read(reinterpret_cast<char *>(&buffer), 1);
    return buffer;
}
uint32_t _readInt(std::istream& in) {
    uint8_t buffer[4];
    in.read(reinterpret_cast<char *>(buffer), 4);

    return buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
}

bool _readBool(std::istream& in) {
    if (_readByte(in) > 0) return true;
    return false;
}

float _readFloat(std::istream& in) {
    uint32_t b4float = _readInt(in);
    return *reinterpret_cast<float*>(&b4float);
}

std::string _readFixed(std::istream& in, std::streamsize len) {
    char buffer[len + 1];
    in.read(buffer, len);
    buffer[len] = 0;
    return {buffer};
}

std::string _readUTF(std::istream& in) {
    uint16_t len = _readShort(in);
    return _readFixed(in, len);
}

OctNode _convert_octnode(uint8_t a) {
    return {
        ((a >> 7 & 1) > 0),
        static_cast<PearlLandState>(static_cast<uint8_t>(a >> 4 & 7)),
        static_cast<CollisionState>(static_cast<uint8_t>(a & 15))
    };
}

void AlgorithmSettings::ReadAlgorithmSettings(std::istream& in) {
    rootCompound = nbt::io::read_compound(in).second;

    auto& version = rootCompound->at("version");
    if (version.get_type() != nbt::tag_type::Int) {
        throw ("Invalid version: no version in algorithm settings");
    }
    if (version.as<nbt::tag_int>().get() != 2) {
        throw ("Invalid algorithm settings version: 2");
    }

    enderpearl = rootCompound->at("enderpearl").as<nbt::tag_byte>().get() != 0;
    tntpearl = rootCompound->at("tntpearl").as<nbt::tag_byte>().get() != 0;
    stonkdown = rootCompound->at("stonkDown").as<nbt::tag_byte>().get() != 0;
    stonkechest = rootCompound->at("stonkEChest").as<nbt::tag_byte>().get() != 0;
    stonkteleport = rootCompound->at("stonkTeleport").as<nbt::tag_byte>().get() != 0;
    etherwarp = rootCompound->at("routeEtherwarp").as<nbt::tag_byte>().get() != 0;
    maxStonkLen = rootCompound->at("maxStonk").as<nbt::tag_int>().get();
    etherwarpRadius = rootCompound->at("etherwarpRadius").as<nbt::tag_int>().get();
    etherwarpLeeway = rootCompound->at("etherwarpLeeway").as<nbt::tag_double>().get();
    etherwarpOffset = rootCompound->at("etherwarpOffset").as<nbt::tag_double>().get();
}

void AlgorithmSettings::WriteAlgorithmSettings(std::ostream &ostream) {
    nbt::io::write_tag("", *rootCompound, ostream);
}

void PathfindRequest::ReadRequest(std::istream& in) {
//    std::ifstream infile(file, std::ios_base::binary);

    std::string magicVal = _readFixed(in, 8);
    if (magicVal != "DGPFREQ2") {
        throw ("Invalid Magic Value: Expected DGPFREQ2, got "+magicVal);
    }

    uint32_t version = _readInt(in);
    if (version != 1) {
        throw ("Invalid version: Expected 1, got "+version);
    }

    id = _readUTF(in);
    idhash = _readUTF(in);
    roomuuid = _readUTF(in);
    roomname = _readUTF(in);
    roomstate = _readUTF(in);

    blockWorld.xLen = _readInt(in);
    blockWorld.zLen = _readInt(in);
    blockWorld.yLen = _readInt(in);

    octNodeWorld.xLen = blockWorld.xLen * 2;
    octNodeWorld.zLen = blockWorld.yLen * 2;
    octNodeWorld.yLen = blockWorld.zLen * 2;

    magicVal = _readFixed(in, 4);
    if (magicVal != "ALGO") {
        throw ("Invalid Magic Value: Expected ALGO, got "+magicVal);
    }
    // instead read nbt.
    settings.ReadAlgorithmSettings(in);

    magicVal = _readFixed(in, 4);
    if (magicVal != "TRGT") {
        throw ("Invalid Magic Value: Expected TRGT, got "+magicVal);
    }

    uint32_t target_size = _readInt(in);
    for (uint32_t i = 0; i < target_size; i++) {
        auto x = static_cast<int32_t>(_readInt(in));
        auto y = static_cast<int32_t>(_readInt(in));
        auto z = static_cast<int32_t>(_readInt(in));

        target.push_back({x,y,z});
    }

    magicVal = _readFixed(in, 4);
    if (magicVal != "WRLD") {
        throw ("Invalid Magic Value: Expected WRLD, got "+magicVal);
    }

//    boost::iostreams::filtering_istream compressed_infile;
//    compressed_infile.push(boost::iostreams::zlib_decompressor());
//    compressed_infile.push(in);

    blockWorld.blocks.resize(blockWorld.xLen * blockWorld.yLen * blockWorld.zLen);
    for (int i = 0; i < blockWorld.xLen * blockWorld.yLen * blockWorld.zLen; i++) {
        blockWorld.blocks[i] = Block{_readByte(in), _readByte(in)};
    }


    magicVal = _readFixed(in, 4);
    if (magicVal != "CLPL") {
        throw ("Invalid Magic Value: Expected CLPL, got "+magicVal);
    }

//    boost::iostreams::filtering_istream compressed_infile2;
//    compressed_infile2.push(boost::iostreams::zlib_decompressor());
//    compressed_infile2.push(in);
    octNodeWorld.nodes.resize(octNodeWorld.xLen * octNodeWorld.yLen * octNodeWorld.zLen);
    for (int i = 0; i < octNodeWorld.xLen * octNodeWorld.yLen * octNodeWorld.zLen; i++) {
        octNodeWorld.nodes[i] = _convert_octnode(_readByte(in));
    }
}


