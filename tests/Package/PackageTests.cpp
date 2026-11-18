#include "Precomp.h"
#include "TestRunner.h"
#include "Package/PackageStream.h"
#include "Utils/File.h"
#include <cstring>

namespace
{
	// Read-only File over a byte buffer, so PackageStream can be driven without a Package on disk.
	class MemoryFile : public File
	{
	public:
		explicit MemoryFile(std::vector<uint8_t> bytes) : Bytes(std::move(bytes)) {}

		int64_t size() override { return (int64_t)Bytes.size(); }

		void read(void* data, size_t count) override
		{
			if (Pos + count > Bytes.size())
				throw std::runtime_error("MemoryFile: read past end");
			memcpy(data, Bytes.data() + Pos, count);
			Pos += count;
		}

		void write(const void*, size_t) override { throw std::runtime_error("MemoryFile is read-only"); }

		void seek(int64_t offset, SeekPoint origin) override
		{
			int64_t base = origin == SeekPoint::begin ? 0 : origin == SeekPoint::current ? (int64_t)Pos : (int64_t)Bytes.size();
			Pos = (size_t)(base + offset);
		}

		uint64_t tell() override { return Pos; }

	private:
		std::vector<uint8_t> Bytes;
		size_t Pos = 0;
	};

	int32_t DecodeIndex(std::vector<uint8_t> bytes, uint64_t* consumed = nullptr)
	{
		PackageStream stream(nullptr, std::make_shared<MemoryFile>(std::move(bytes)));
		int32_t value = stream.ReadIndex();
		if (consumed)
			*consumed = stream.Tell();
		return value;
	}
}

// Golden vectors derived by hand from the algorithm in Docs/PackageFormat.md:
// byte 0 = sign(bit 7) | more(bit 6) | low 6 bits, later bytes = more(bit 7) | 7 bits.
TEST(CompactIndexSingleByte)
{
	CHECK_EQ(DecodeIndex({ 0x00 }), 0);
	CHECK_EQ(DecodeIndex({ 0x01 }), 1);
	CHECK_EQ(DecodeIndex({ 0x3F }), 63);
	CHECK_EQ(DecodeIndex({ 0x81 }), -1);
	CHECK_EQ(DecodeIndex({ 0xBF }), -63);
}

TEST(CompactIndexMultiByte)
{
	CHECK_EQ(DecodeIndex({ 0x40, 0x01 }), 64);
	CHECK_EQ(DecodeIndex({ 0x64, 0x01 }), 100);
	CHECK_EQ(DecodeIndex({ 0xC0, 0x01 }), -64);
	CHECK_EQ(DecodeIndex({ 0x40, 0x80, 0x01 }), 8192);
	CHECK_EQ(DecodeIndex({ 0x7F, 0xFF, 0xFF, 0xFF, 0x0F }), 0x7FFFFFFF);
	CHECK_EQ(DecodeIndex({ 0xFF, 0xFF, 0xFF, 0xFF, 0x0F }), -0x7FFFFFFF);
}

TEST(CompactIndexConsumesExactlyItsBytes)
{
	uint64_t consumed = 0;
	DecodeIndex({ 0x3F, 0xAA, 0xAA }, &consumed);
	CHECK_EQ(consumed, (uint64_t)1);
	DecodeIndex({ 0x64, 0x01, 0xAA }, &consumed);
	CHECK_EQ(consumed, (uint64_t)2);
	DecodeIndex({ 0x40, 0x80, 0x01, 0xAA }, &consumed);
	CHECK_EQ(consumed, (uint64_t)3);
}

TEST(CompactIndexTruncatedInputThrows)
{
	CHECK_THROWS(DecodeIndex({ 0x40 })); // continuation flag set but no following byte
	CHECK_THROWS(DecodeIndex({}));
}
