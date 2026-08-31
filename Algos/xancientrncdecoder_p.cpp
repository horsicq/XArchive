/* Copyright (C) Teemu Suutari */

#include <algorithm>

#include "xancientrncdecoder_p.h"
#include "xancienthuffman_p.h"
#include "xancientinputstream_p.h"
#include "xancientoutputstream_p.h"
#include "xancientcrc16_p.h"
#include "xancientoverflow_p.h"
#include "xancientcommon_p.h"

#include "xancientvlc_p.h"

#include <array>
#include <vector>

// This allows decompression of pc compressed files from unonfficial (and unpatched) compressor
// PC games do not need chunk count, and are happy to read these files.
// Official tools put it and amiga decompressors require it
#define ALLOW_MISSING_CHUNKS 1

namespace XAncientPrivate
{

bool RncDecoder::detectHeader(uint32_t hdr,uint32_t footer) noexcept
{
	return hdr==FourCC("RNC\001") || hdr==FourCC("RNC\002")
		|| hdr==FourCC("...\001");		// Total Carnage
}

std::shared_ptr<CodecDecoder> RncDecoder::create(const ByteBuffer &packedData,bool exactSizeKnown,bool verify)
{
	return std::make_shared<RncDecoder>(packedData,verify);
}

RncDecoder::RncDecoder(const ByteBuffer &packedData,bool verify) :
	_packedData{packedData}
{
	uint32_t hdr{packedData.readBE32(0)};
	_rawSize=packedData.readBE32(4);
	_packedSize=packedData.readBE32(8);
	if (!_rawSize || !_packedSize ||
		_rawSize>getMaxRawSize() || _packedSize>getMaxPackedSize())
			throw InvalidFormatError();

	bool verified{false};
	if (hdr==FourCC("RNC\001"))
	{
		// now detect between old and new version
		// since the old and the new version share the same id, there is no foolproof way
		// to tell them apart. It is easier to prove that it is not something by finding
		// specific invalid bitstream content.

		// CRC-first detection: the packed-stream CRC stored at offset 14 exists only
		// in the new format (the old format has no CRC field there), so a match is a
		// definitive "this is the new format" signal. Testing it FIRST is required for
		// "locked"/encrypted new-format streams, whose leading bytes otherwise look like
		// a valid old-format stream and get misclassified by the filler-bit heuristic.
		// well, this is silly though but lets assume someone has made old format RNC1 with total size less than 19
		if (packedData.size()<=18U)
		{
			_ver=Version::RNC1Old;
		}
		else if (_packedData.size()>=BoundsCheck::sum(_packedSize,18U) && CRC16(_packedData,18U,_packedSize,0)==packedData.readBE16(14U))
		{
			_ver=Version::RNC1New;
			verified=true;
		}
		else
		{
			// No CRC match: fall back to the old vs new stream-start heuristic.
			uint8_t oldStreamStart{packedData.read8(_packedSize+11U)};
			_ver = (oldStreamStart&0x80U) ? Version::RNC1Old : Version::RNC1New;
		}
	} else if (hdr==FourCC("RNC\002")) {
		// ...and detect between the new and old format of RNC2.
		// Same CRC-first reasoning as the RNC1 branch above; RNC2's old-format
		// stream-start byte lives at _packedSize+10U (not +11U).
		if (packedData.size()<=18U)
		{
			_ver=Version::RNC2Old;
		}
		else if (_packedData.size()>=BoundsCheck::sum(_packedSize,18U) && CRC16(_packedData,18U,_packedSize,0)==packedData.readBE16(14U))
		{
			_ver=Version::RNC2New;
			verified=true;
		}
		else
		{
			// RNC2Old is very similar to RNC1Old, RNC2 has padding at start which makes things more complex
			uint8_t oldStreamStart{packedData.read8(_packedSize+10U)};
			_ver = (oldStreamStart&0x80U) ? Version::RNC2Old : Version::RNC2New;
		}
	} else if (hdr==FourCC("...\001")) {
		_ver=Version::RNC1New;
	} else throw InvalidFormatError();

	uint32_t hdrSize{(_ver==Version::RNC1Old || _ver==Version::RNC2Old)?12U:18U};
	if (BoundsCheck::sum(_packedSize,hdrSize)>packedData.size())
		throw InvalidFormatError();

	if (_ver!=Version::RNC1Old && _ver!=Version::RNC2Old)
	{
		_rawCRC=packedData.readBE16(12U);
		_chunks=packedData.read8(17U);
		if (verify && !verified)
		{
			if (CRC16(_packedData,18,_packedSize,0)!=packedData.readBE16(14))
				throw VerificationError();
		}
	}
}

const std::string &RncDecoder::getName() const noexcept
{
	static std::string names[4]={
		{"RNC1: Rob Northen RNC1 Compressor (old)"},
		{"RNC1: Rob Northen RNC1 Compressor"},
		{"RNC2: Rob Northen RNC2 Compressor (old)"},
		{"RNC2: Rob Northen RNC2 Compressor"}};
	return names[static_cast<uint32_t>(_ver)];
}

size_t RncDecoder::getPackedSize() const noexcept
{
	if (_ver==Version::RNC1Old || _ver==Version::RNC2Old) return _packedSize+12U;
		else return _packedSize+18U;
}

size_t RncDecoder::getRawSize() const noexcept
{
	return _rawSize;
}

void RncDecoder::decompressImpl(ByteBuffer &rawData,bool verify)
{
	if (rawData.size()<_rawSize)
		throw DecompressionError();

	switch (_ver)
	{
		case Version::RNC1Old:
		return RNCDecompressOld(rawData,verify,false);

		case Version::RNC1New:
		return RNC1DecompressNew(rawData,verify);

		case Version::RNC2Old:
		return RNCDecompressOld(rawData,verify,true);

		case Version::RNC2New:
		return RNC2DecompressNew(rawData,verify);

		default:
		throw DecompressionError();
	}
}

void RncDecoder::RNCDecompressOld(ByteBuffer &rawData,bool verify,bool rnc2)
{
	BackwardInputStream inputStream{_packedData,12U,_packedSize+12U};
	MSBBitReader<BackwardInputStream> bitReader{inputStream};
	struct ReadBits
	{
		MSBBitReader<BackwardInputStream> &bitReader;

		uint32_t operator()(uint32_t count) const
		{
			return bitReader.readBits8(count);
		}
	};
	struct ReadBit
	{
		MSBBitReader<BackwardInputStream> &bitReader;

		uint32_t operator()() const
		{
			return bitReader.readBits8(1);
		}
	};
	struct ReadByte
	{
		BackwardInputStream &inputStream;

		uint8_t operator()() const
		{
			return inputStream.readByte();
		}
	};
	ReadBits readBits{bitReader};
	ReadBit readBit{bitReader};
	ReadByte readByte{inputStream};

	uint32_t lastDistanceBits{12};
	uint32_t lastLengthBits{10};
	if (rnc2)
	{
		uint32_t tmp=readByte()+1U;
		lastDistanceBits=tmp&0xfU;
		lastLengthBits=(tmp>>4U)+1U;
	}

	// the anchor-bit does not seem always to be at the correct place
	{
		uint8_t halfByte{readByte()};
		for (uint32_t i=0;i<7;i++)
			if (halfByte&(1<<i))
			{
				bitReader.reset(halfByte>>(i+1),7-i);
				break;
			}
	}

	BackwardOutputStream outputStream{rawData,0,_rawSize};

	HuffmanDecoder<uint8_t> lengthDecoder
	{
		HuffmanCode{1,0b0000,uint8_t{0}},
		HuffmanCode{2,0b0010,uint8_t{1}},
		HuffmanCode{3,0b0110,uint8_t{2}},
		HuffmanCode{4,0b1110,uint8_t{3}},
		HuffmanCode{4,0b1111,uint8_t{4}}
	};

	HuffmanDecoder<uint8_t> distanceDecoder
	{
		HuffmanCode{1,0b00,uint8_t{1}},
		HuffmanCode{2,0b10,uint8_t{0}},
		HuffmanCode{2,0b11,uint8_t{2}}
	};

	VariableLengthCodeDecoder litVlcDecoder1{1,1,2,2,3,10};
	VariableLengthCodeDecoder litVlcDecoder2{1,1,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
	VariableLengthCodeDecoder lengthVlcDecoder{0,0,1,2,lastLengthBits};
	VariableLengthCodeDecoder distanceVlcDecoder{5,8,lastDistanceBits};

	for (;;)
	{
		uint32_t litLength{rnc2?litVlcDecoder2.decodeCascade(readBits):litVlcDecoder1.decodeCascade(readBits)};
		for (uint32_t i=0;i<litLength;i++) outputStream.writeByte(readByte());
	
		// the only way to successfully end the loop!
		if (outputStream.eof()) break;

		uint32_t count{lengthVlcDecoder.decode(readBits,lengthDecoder.decode(readBit))+2U};

		uint32_t distance;
		if (count!=2U)
		{
			distance=distanceVlcDecoder.decode(readBits,distanceDecoder.decode(readBit));
		} else {
			if (!readBit()) distance=readBits(6U);
				else distance=readBits(9U)+64U;
		}

		outputStream.copy((distance)?distance+count-1U:1U,count);
	}
}

void RncDecoder::RNC1DecompressNew(ByteBuffer &rawData,bool verify)
{
	ForwardInputStream inputStream{_packedData,18U,_packedSize+18U,1U};
	LSBBitReader<ForwardInputStream> bitReader{inputStream};
	struct ReadBits
	{
		LSBBitReader<ForwardInputStream> &bitReader;

		uint32_t operator()(uint32_t count) const
		{
			return bitReader.readBitsLE16(count);
		}
	};
	struct ReadBit
	{
		LSBBitReader<ForwardInputStream> &bitReader;

		uint32_t operator()() const
		{
			return bitReader.readBitsLE16(1);
		}
	};
	struct ReadByte
	{
		ForwardInputStream &inputStream;

		uint8_t operator()() const
		{
			return inputStream.readByte();
		}
	};
	ReadBits readBits{bitReader};
	ReadBit readBit{bitReader};
	ReadByte readByte{inputStream};

	ForwardOutputStream outputStream{rawData,0,_rawSize};

	// "Locked" (encrypted) stream support (ProPack -k). Each literal byte is XORed
	// with the low 8 bits of a 16-bit key; the key is rotated right by one bit after
	// every non-empty literal run. Match bytes copy already-produced (still-encrypted)
	// bytes, so they inherit the literal-run index of their copy source. We decode the
	// stream verbatim (encrypted) while recording, per output byte, the run index that
	// governs its decryption, then recover the key and decrypt in a single pass below.
	// runIndex is only populated when the encrypted flag is set (zero overhead for the
	// common plain streams).
	bool encrypted{false};
	std::vector<uint32_t> runIndex;
	uint32_t runCounter{0};
	size_t trackPos{0};

	typedef HuffmanDecoder<uint32_t> RNC1HuffmanDecoder;

	// helpers
	struct ReadHuffmanTable
	{
		ReadBits &readBits;

		void operator()(RNC1HuffmanDecoder &dec) const
		{
			uint32_t length{readBits(5)};
			if (!length) return;
			std::array<uint8_t,31> lengthTable;
			for (uint32_t i=0;i<length;i++)
				lengthTable[i]=readBits(4);

			dec.createOrderlyHuffmanTable(lengthTable,length);
		}
	};

	struct HuffmanDecode
	{
		ReadBit &readBit;
		ReadBits &readBits;

		uint32_t operator()(const RNC1HuffmanDecoder &dec) const
		{
			// this is kind of non-specced
			uint32_t ret{dec.decode(readBit)};
			if (ret>=2U)
				ret=(1U<<(ret-1U))|readBits(ret-1U);
			return ret;
		}
	};

	struct ProcessLiterals
	{
		HuffmanDecode &huffmanDecode;
		ForwardOutputStream &outputStream;
		ReadByte &readByte;
		bool &encrypted;
		std::vector<uint32_t> &runIndex;
		size_t &trackPos;
		uint32_t &runCounter;

		void operator()(const RNC1HuffmanDecoder &dec) const
		{
			uint32_t litLength{huffmanDecode(dec)};
			for (uint32_t i=0;i<litLength;i++)
			{
				// writeByte throws before it would overrun, so trackPos<_rawSize here.
				outputStream.writeByte(readByte());
				if (encrypted) runIndex[trackPos++]=runCounter;
			}
			// Empty literal runs do not advance the key rotation.
			if (encrypted && litLength) runCounter++;
		}
	};
	ReadHuffmanTable readHuffmanTable{readBits};
	HuffmanDecode huffmanDecode{readBit,readBits};
	ProcessLiterals processLiterals{huffmanDecode,outputStream,readByte,encrypted,runIndex,trackPos,runCounter};

	// Two leading flag bits: bit0 = decompress-in-place, bit1 = stream is encrypted.
	uint32_t flags{readBits(2)};
	encrypted=(flags&2U)!=0U;
	if (encrypted) runIndex.resize(_rawSize);
#ifdef ALLOW_MISSING_CHUNKS
	while (!outputStream.eof())
#else
	for (uint8_t chunks=0;chunks<_chunks;chunks++)
#endif
	{
		RNC1HuffmanDecoder litDecoder,distanceDecoder,lengthDecoder;
		readHuffmanTable(litDecoder);
		readHuffmanTable(distanceDecoder);
		readHuffmanTable(lengthDecoder);
		uint32_t count{readBits(16)};

		for (uint32_t sub=1;sub<count;sub++)
		{
			processLiterals(litDecoder);
			uint32_t distance{huffmanDecode(distanceDecoder)};
			uint32_t subCount{huffmanDecode(lengthDecoder)};
			distance++;
			subCount+=2;
			// copy throws before it would overrun, so the trackPos loop stays in bounds.
			outputStream.copy(distance,subCount);
			if (encrypted)
				for (uint32_t i=0;i<subCount;i++,trackPos++)
					runIndex[trackPos]=runIndex[trackPos-distance];
		}
		processLiterals(litDecoder);
	}

	if (!outputStream.eof())
		throw DecompressionError();

	// Recover and apply the decryption key for locked streams. The only integrity
	// value the format carries is the 16-bit unpacked CRC, and the key enters the CRC
	// linearly (each key bit XORs a fixed set of output bytes), so the key is a GF(2)
	// linear function of the target CRC. We reconstruct it here instead of requiring a
	// user-supplied password.
	if (encrypted)
	{
		uint16_t base{CRC16(rawData,0,_rawSize,0)};
		if (base!=_rawCRC)
		{
			struct RotateRight16
			{
				uint32_t operator()(uint32_t key,uint32_t n) const
				{
					n&=15U;
					return ((key>>n)|(key<<(16U-n)))&0xffffU;
				}
			};
			RotateRight16 ror16;

			// deltas[j] = CRC contribution of key bit j. Decrypting a byte at run index
			// r XORs it with bit ((j-r) mod 16) of the key, but only bits landing in the
			// low byte (position < 8) actually reach the output byte. Accumulate the 16
			// per-bit CRCs in one streaming pass over the output.
			std::array<uint16_t,16> deltas{};
			for (size_t i=0;i<_rawSize;i++)
			{
				uint32_t r{runIndex[i]&15U};
				for (uint32_t j=0;j<16;j++)
				{
					uint32_t p{(static_cast<uint32_t>(j)-r)&15U};
					uint8_t b{(p<8U)?static_cast<uint8_t>(1U<<p):uint8_t{0}};
					deltas[j]=CRC16Byte(b,deltas[j]);
				}
			}

			struct PredictCRC
			{
				uint16_t base;
				const std::array<uint16_t,16> &deltas;

				uint16_t operator()(uint32_t key) const
				{
					uint16_t x{base};
					for (uint32_t j=0;j<16;j++)
						if (key&(1U<<j)) x^=deltas[j];
					return x;
				}
			};
			PredictCRC predicted{base,deltas};

			// The 16-bit CRC constraint is often rank-deficient: several distinct keys
			// reproduce it, but only the real password gives the true plaintext; the
			// others yield CRC-valid-but-wrong output. Recovering the wrong key would
			// SILENTLY CORRUPT the file, which is worse than reporting it unrecoverable.
			// So decrypt only when the key is safe to trust:
			//   (1) a well-known reused ProPack collection password satisfies the CRC, or
			//   (2) exactly one key in the whole 16-bit space satisfies it (unambiguous).
			// When several keys match and none is a known password, refuse to guess and
			// fall through to VerificationError (honest "cannot decrypt"). This recovers
			// every locked file whose password is known or uniquely determined and never
			// emits unverifiable plaintext. RNC2 "locked" streams are not implemented
			// (none exist in-corpus).
			static const uint16_t knownKeys[]{0x04d2U,0x1984U,0x5ed0U};
			uint32_t key{0};
			bool haveKey{false};
			for (uint16_t k : knownKeys)
				if (predicted(k)==_rawCRC) { key=k; haveKey=true; break; }
			if (!haveKey)
			{
				uint32_t nMatches{0};
				for (uint32_t k=1;k<0x10000U && nMatches<2U;k++)
					if (predicted(k)==_rawCRC) { key=k; ++nMatches; }
				haveKey=(nMatches==1U);  // unique key only; ambiguous stays refused
			}

			// If no trustworthy key was found (genuinely damaged, or ambiguous with an
			// unknown password) the stream is left encrypted and the verification check
			// below reports it as before.
			if (haveKey)
				for (size_t i=0;i<_rawSize;i++)
					rawData[i]^=static_cast<uint8_t>(ror16(key,runIndex[i])&0xffU);
		}
	}

	if (verify && CRC16(rawData,0,_rawSize,0)!=_rawCRC)
		throw VerificationError();
}

void RncDecoder::RNC2DecompressNew(ByteBuffer &rawData,bool verify)
{
	ForwardInputStream inputStream{_packedData,18U,_packedSize+18U};
	MSBBitReader<ForwardInputStream> bitReader{inputStream};
	struct ReadBits
	{
		MSBBitReader<ForwardInputStream> &bitReader;

		uint32_t operator()(uint32_t count) const
		{
			return bitReader.readBits8(count);
		}
	};
	struct ReadBit
	{
		MSBBitReader<ForwardInputStream> &bitReader;

		uint32_t operator()() const
		{
			return bitReader.readBits8(1);
		}
	};
	struct ReadByte
	{
		ForwardInputStream &inputStream;

		uint8_t operator()() const
		{
			return inputStream.readByte();
		}
	};
	ReadBits readBits{bitReader};
	ReadBit readBit{bitReader};
	ReadByte readByte{inputStream};

	ForwardOutputStream outputStream{rawData,0,_rawSize};

	// Huffman decoding
	enum class Cmd
	{
		LIT=0,	// 0, Literal
		MOV,	// 10, Move bytes + length + distance, Get bytes if length=9 + 4bits
		MV2,	// 110, Move 2 bytes
		MV3,	// 1110, Move 3 bytes
		CND	// 1111, Conditional copy, or EOF
		
	};

	HuffmanDecoder<Cmd> cmdDecoder
	{
		HuffmanCode{1,0b0000,Cmd::LIT},
		HuffmanCode{2,0b0010,Cmd::MOV},
		HuffmanCode{3,0b0110,Cmd::MV2},
		HuffmanCode{4,0b1110,Cmd::MV3},
		HuffmanCode{4,0b1111,Cmd::CND}
	};

	/* length of 9 is a marker for literals */
	HuffmanDecoder<uint8_t> lengthDecoder
	{
		HuffmanCode{2,0b000,uint8_t{4}},
		HuffmanCode{2,0b010,uint8_t{5}},
		HuffmanCode{3,0b010,uint8_t{6}},
		HuffmanCode{3,0b011,uint8_t{7}},
		HuffmanCode{3,0b110,uint8_t{8}},
		HuffmanCode{3,0b111,uint8_t{9}}
	};
	
	HuffmanDecoder<uint8_t> distanceDecoder
	{
		HuffmanCode{1,0b000000,uint8_t{0}},
		HuffmanCode{3,0b000110,uint8_t{1}},
		HuffmanCode{4,0b001000,uint8_t{2}},
		HuffmanCode{4,0b001001,uint8_t{3}},
		HuffmanCode{5,0b010101,uint8_t{4}},
		HuffmanCode{5,0b010111,uint8_t{5}},
		HuffmanCode{5,0b011101,uint8_t{6}},
		HuffmanCode{5,0b011111,uint8_t{7}},
		HuffmanCode{6,0b101000,uint8_t{8}},
		HuffmanCode{6,0b101001,uint8_t{9}},
		HuffmanCode{6,0b101100,uint8_t{10}},
		HuffmanCode{6,0b101101,uint8_t{11}},
		HuffmanCode{6,0b111000,uint8_t{12}},
		HuffmanCode{6,0b111001,uint8_t{13}},
		HuffmanCode{6,0b111100,uint8_t{14}},
		HuffmanCode{6,0b111101,uint8_t{15}}
	};

	// helpers
	struct ReadDistance
	{
		const HuffmanDecoder<uint8_t> &distanceDecoder;
		ReadBit &readBit;
		ReadByte &readByte;

		uint32_t operator()() const
		{
			uint8_t distMult{distanceDecoder.decode(readBit)};
			uint8_t distByte{readByte()};
			return (uint32_t(distByte)|(uint32_t(distMult)<<8))+1;
		}
	};
	
	struct MoveBytes
	{
		ForwardOutputStream &outputStream;

		void operator()(uint32_t distance,uint32_t count) const
		{
			if (!count)
				throw DecompressionError();
			outputStream.copy(distance,count);
		}
	};
	ReadDistance readDistance{distanceDecoder,readBit,readByte};
	MoveBytes moveBytes{outputStream};

	readBit();
	readBit();
	uint8_t foundChunks{0};
	bool done{false};
	while (!done && foundChunks<_chunks)
	{
		Cmd cmd{cmdDecoder.decode(readBit)};
		switch (cmd) {
			case Cmd::LIT:
			outputStream.writeByte(readByte());
			break;

			case Cmd::MOV:
			{
				uint8_t count{lengthDecoder.decode(readBit)};
				if (count!=9)
					moveBytes(readDistance(),count);
				else {
					uint32_t rep{(readBits(4U)+3U)*4U};
					for (uint32_t i=0;i<rep;i++)
						outputStream.writeByte(readByte());
				}
			}
			break;

			case Cmd::MV2:
			moveBytes(uint32_t{readByte()}+1U,2U);
			break;

			case Cmd::MV3:
			moveBytes(readDistance(),3U);
			break;

			case Cmd::CND:
			{
				uint8_t count{readByte()};
				if (count)
					moveBytes(readDistance(),uint32_t{count}+8U);
				else {
					foundChunks++;
					done=!readBit();
				}
				
			}			
			break;
		}
	}

	if (!outputStream.eof() || _chunks!=foundChunks)
		throw DecompressionError();
	if (verify && CRC16(rawData,0,_rawSize,0)!=_rawCRC)
		throw VerificationError();
}

}
