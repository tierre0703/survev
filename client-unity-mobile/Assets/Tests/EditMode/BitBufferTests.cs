// Captured-frame tests for Core/BitBuffer.
//
// The protocol must be bit-exact against the server. These tests establish the
// harness; fixtures are captured from the web client / server (see plan.md M2)
// and added here as byte arrays. Until then, this verifies the primitive
// roundtrip semantics the TS BitBuffer guarantees.
using NUnit.Framework;
using Survev.Core;

namespace Survev.Tests.EditMode
{
    public class BitBufferTests
    {
        [Test]
        public void WriteReadBits_RoundTripsLsbFirst()
        {
            var writer = new BitBuffer();
            writer.WriteBits(0b101, 3);
            writer.WriteBits(0b1100, 4);
            writer.WriteBits(0, 1); // pad to a byte boundary like the TS writer
            byte[] bytes = writer.ToArray();

            var reader = FromBytes(bytes);
            Assert.AreEqual(0b101, reader.ReadBits(3));
            Assert.AreEqual(0b1100, reader.ReadBits(4));
            Assert.AreEqual(0, reader.ReadBits(1));
        }

        [Test]
        public void UInt32_RoundTrips()
        {
            var writer = new BitBuffer();
            writer.WriteUInt32(0xDEADBEEF);
            byte[] bytes = writer.ToArray();

            var reader = FromBytes(bytes);
            Assert.AreEqual(0xDEADBEEFu, reader.ReadUInt32());
        }

        [Test]
        public void Float_RoundTrips()
        {
            var writer = new BitBuffer();
            writer.WriteFloat(3.14159f);
            byte[] bytes = writer.ToArray();

            var reader = FromBytes(bytes);
            Assert.AreEqual(3.14159f, reader.ReadFloat(), 1e-6f);
        }

        [Test]
        public void String_RoundTripsUtf8()
        {
            var writer = new BitBuffer();
            writer.WriteString("survev \u00e9\u00e8");
            byte[] bytes = writer.ToArray();

            var reader = FromBytes(bytes);
            Assert.AreEqual("survev \u00e9\u00e8", reader.ReadString());
        }

        [Test]
        public void Bool_RoundTrips()
        {
            var writer = new BitBuffer();
            writer.WriteBool(true);
            writer.WriteBool(false);
            writer.WriteBool(true);
            byte[] bytes = writer.ToArray();

            var reader = FromBytes(bytes);
            Assert.IsTrue(reader.ReadBool());
            Assert.IsFalse(reader.ReadBool());
            Assert.IsTrue(reader.ReadBool());
        }

        private static BitBuffer FromBytes(byte[] bytes)
        {
            // Helper for tests: replay captured bytes into a fresh reader. The
            // real captured-frame fixtures will use a read-only ctor once the
            // protocol tests land.
            var reader = new BitBuffer();
            for (int i = 0; i < bytes.Length; i++)
            {
                reader.WriteUInt8(bytes[i]);
            }
            reader.Rewind();
            return reader;
        }
    }
}