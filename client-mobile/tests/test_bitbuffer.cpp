#include "TestFramework.h"
#include "TestHelpers.h"

using namespace surv;
using namespace surv_test;

TEST(primitives) {
    Buffer b;
    NetBitStream& s = b.stream;
    s.writeBoolean(true);
    s.writeUint8(0xab);
    s.writeInt16(-1234);
    s.writeUint16(0xbeef);
    s.writeInt32(-2000000000);
    s.writeUint32(0xffffffffu);
    s.writeFloat(3.14f, 0.0f, 10.0f, 8);
    s.writeFloat32(1.5f);
    s.writeFloat64(12345.6789);
    s.writeASCIIString("hello");
    s.writeUTF8String("héllo");
    s.writeUint8(0x00);
    CHECK(matchReference("primitives", b.hex()));
}

TEST(vecs) {
    Buffer b;
    NetBitStream& s = b.stream;
    s.writeMapPos(Vec2(100.5f, 200.25f));
    s.writeUnitVec(Vec2(0.707f, 0.707f), 8);
    s.writeVec32(Vec2(-5.5f, 6.25f));
    CHECK(matchReference("vecs", b.hex()));
}

TEST(string_fixed_align) {
    Buffer b;
    NetBitStream& s = b.stream;
    s.writeString("player1", 16);
    s.writeAlignToNextByte();
    s.writeUint8(7);
    CHECK(matchReference("string-fixed-align", b.hex()));
}

TEST(bitview_roundtrip) {
    std::vector<uint8_t> buf(64, 0);
    BitStream s(buf.data(), buf.size());
    s.writeBits(0b101u, 3);
    s.writeBits(0b11111111u, 8);
    s.writeBits(0xdeadbeefu, 32);
    s.writeBoolean(true);

    BitStream r(buf.data(), buf.size());
    CHECK_EQ(r.readBits(3), 0b101u);
    CHECK_EQ(r.readBits(8), 0xffu);
    CHECK_EQ(r.readBits(32), 0xdeadbeefu);
    CHECK(r.readBoolean());
}

TEST(bitview_signed) {
    std::vector<uint8_t> buf(16, 0);
    BitStream s(buf.data(), buf.size());
    s.writeInt8(-5);
    s.writeInt16(-1234);
    s.writeInt32(-2000000000);

    BitStream r(buf.data(), buf.size());
    CHECK_EQ(r.readInt8(), -5);
    CHECK_EQ(r.readInt16(), -1234);
    CHECK_EQ(r.readInt32(), -2000000000);
}

TEST(bitview_float64) {
    std::vector<uint8_t> buf(16, 0);
    BitStream s(buf.data(), buf.size());
    const double val = 12345.6789;
    s.writeFloat64(val);
    BitStream r(buf.data(), buf.size());
    CHECK(r.readFloat64() == val);
}