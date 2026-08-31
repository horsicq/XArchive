/* Copyright (C) Teemu Suutari */

#include <cstdint>
#include <cstring>

#include "xancientdmsdecoder_p.h"

#include "xancienthuffman_p.h"
#include "xancientdynamichuffman_p.h"
#include "xancientinputstream_p.h"
#include "xancientoutputstream_p.h"
#include "xancientvlc_p.h"
#include "xancientmemorybuffer_p.h"
#include "xancientcrc16_p.h"
#include "xancientoverflow_p.h"
#include "xancientcommon_p.h"


namespace XAncientPrivate
{

bool DmsDecoder::detectHeader(uint32_t hdr,uint32_t footer) noexcept
{
	return hdr==FourCC("DMS!");
}

std::shared_ptr<CodecDecoder> DmsDecoder::create(const ByteBuffer &packedData,bool exactSizeKnown,bool verify)
{
	return std::make_shared<DmsDecoder>(packedData,verify);
}

DmsDecoder::DmsDecoder(const ByteBuffer &packedData,bool verify) :
	_packedData{packedData}
{
	uint32_t hdr{packedData.readBE32(0)};
	if (!detectHeader(hdr,0) || packedData.size()<56)
		throw InvalidFormatError();

	if (verify && CRC16(packedData,4,50,0)!=packedData.readBE16(54))
		throw VerificationError();

	uint16_t info{packedData.readBE16(10)};
	_isObsfuscated=info&2;				// using 16 bit key is not encryption, it is obsfuscation
	_isHD=info&16;
	if (info&32)
		throw InvalidFormatError();		// MS-DOS disk

	// packed data in header is useless, as is rawsize and track numbers
	// they are not always filled

	if (packedData.readBE16(50)>6)
		throw InvalidFormatError();		// either FMS or unknown

	const std::array<uint32_t,7> contextSizes{0,0,256,16384,16384,4096,8192};

	// now calculate the real packed size, including headers
	uint32_t offset{56};
	uint32_t accountedSize{0};
	uint32_t lastTrackSize{0};
	uint32_t numTracks{0};
	uint32_t minTrack{80};
	uint32_t prevTrack{0};
	while (offset+20<packedData.size())
	{
		if (_packedData.readBE16(offset)!=MultiChar2("TR"))
		{
			// secondary exit criteria, should not be like this, if the header would be trustworthy
			if (!accountedSize)
				throw InvalidFormatError();
			break;
		}
		uint32_t trackNo{_packedData.readBE16(offset+2)};
		// lets not go backwards on tracks!
		if (trackNo<prevTrack) break;

		// header check
		if (verify && CRC16(packedData,offset,18,0)!=packedData.readBE16(offset+18))
			throw VerificationError();

		uint8_t mode{_packedData.read8(offset+13)};
		if (mode>6)
			throw InvalidFormatError();

		_contextBufferSize=std::max(_contextBufferSize,contextSizes[mode]);

		uint8_t flags{_packedData.read8(offset+12)};
		if ((mode>=2 && mode<=4) || (mode>=5 && (flags&4)))
			_tmpBufferSize=std::max(_tmpBufferSize,uint32_t(_packedData.readBE16(offset+8)));
		uint32_t packedChunkLength{packedData.readBE16(offset+6)};
		if (BoundsCheck::sum(offset,20U,packedChunkLength)>packedData.size())
			throw InvalidFormatError();
		if (verify && CRC16(packedData,offset+20,packedChunkLength,0)!=packedData.readBE16(offset+16))
			throw VerificationError();

		if (trackNo<80)
		{
			if (trackNo>=numTracks) lastTrackSize=_packedData.readBE16(offset+10); 
			minTrack=std::min(minTrack,trackNo);
			numTracks=std::max(numTracks,trackNo);
			prevTrack=trackNo;
		}

		offset+=packedChunkLength+20;
		accountedSize+=packedChunkLength;
		// this is the real exit critea, unfortunately
		if (trackNo>=79 && trackNo<0x8000U) break;
	}
	uint32_t trackSize{(_isHD)?22528U:11264U};
	_rawOffset=minTrack*trackSize;
	if (minTrack>=numTracks)
		throw InvalidFormatError();
	_minTrack=minTrack;
	_rawSize=(numTracks-minTrack)*trackSize+lastTrackSize;
	_imageSize=trackSize*80;

	_packedSize=offset;
	if (_packedSize>getMaxPackedSize())
		throw InvalidFormatError();
}

const std::string &DmsDecoder::getName() const noexcept
{
	static std::string name{"DMS: Disk Masher System"};
	return name;
}

size_t DmsDecoder::getPackedSize() const noexcept
{
	return _packedSize;
}

size_t DmsDecoder::getRawSize() const noexcept
{
	return _rawSize;
}

size_t DmsDecoder::getImageSize() const noexcept
{
	return _imageSize;
}

size_t DmsDecoder::getImageOffset() const noexcept
{
	return _rawOffset;
}

void DmsDecoder::decompressImpl(ByteBuffer &rawData,bool verify)
{
	uint32_t restartPosition{0};
	if (!_isObsfuscated)
	{
		decompressImpl(rawData,verify,restartPosition);
	} else {
		while (restartPosition<0x20000U)
		{
			// more than single run here is really rare. It means that first track CRC succeeds
			// but later something else fails
			try
			{
				decompressImpl(rawData,verify,restartPosition);
				return;
			} catch (const ByteBuffer::Error &) {
				// just continue
			} catch (const CodecDecoder::Error &) {
				// just continue
			}
			restartPosition++;
		}
		throw DecompressionError();
	}
}

// TODO: Too much state for a single method. too convoluted
// needs to be split
void DmsDecoder::decompressImpl(ByteBuffer &rawData,bool verify,uint32_t &restartPosition)
{
	if (rawData.size()<_rawSize)
		throw DecompressionError();
	OwnedBuffer contextBuffer{_contextBufferSize};
	OwnedBuffer tmpBuffer{_tmpBufferSize};
	uint32_t limitedDecompress{~0U};

	class ObsfuscatedInputStream
	{
	public:
		ObsfuscatedInputStream(const ByteBuffer &data,uint32_t start,uint32_t length,bool obsfuscate) :
			_inputStream{data,start,BoundsCheck::sum(start,length)},
			_obsfuscate{obsfuscate}
		{
			// nothing needed
		}
		~ObsfuscatedInputStream() noexcept=default;

		void reset(size_t startOffset,size_t endOffset)
		{
			_inputStream.reset(startOffset,endOffset);
		}

		uint8_t readByte()
		{
			uint8_t ch{_inputStream.readByte()};
			if (!_obsfuscate)
			{
				return ch;
			} else {
				uint8_t ret{uint8_t(ch^_passAccumulator)};
				_passAccumulator=(_passAccumulator>>1)+uint16_t(ch);
				return ret;
			}
		}

		void setCode(uint16_t passAccumulator)
		{
			_passAccumulator=passAccumulator;
		}

		bool eof() const { return _inputStream.getOffset()==_inputStream.getEndOffset(); }

	private:
		ForwardInputStream	_inputStream;
		bool			_obsfuscate;
		uint16_t		_passAccumulator{0};
	};

	ObsfuscatedInputStream inputStream{_packedData,0,0,_isObsfuscated};
	MSBBitReader<ObsfuscatedInputStream> bitReader{inputStream};
	struct InitInputStream
	{
		ObsfuscatedInputStream &inputStream;
		MSBBitReader<ObsfuscatedInputStream> &bitReader;

		void operator()(uint32_t start,uint32_t length) const
		{
			inputStream.reset(start,BoundsCheck::sum(start,length));
			bitReader.reset();
		}
	};
	struct FinishStream
	{
		ObsfuscatedInputStream &inputStream;
		const bool &isObsfuscated;
		const uint32_t &limitedDecompress;

		void operator()() const
		{
			if (isObsfuscated && limitedDecompress==~0U)
				while (!inputStream.eof())
					inputStream.readByte();
		}
	};
	struct ReadBits
	{
		MSBBitReader<ObsfuscatedInputStream> &bitReader;

		uint32_t operator()(uint32_t count) const
		{
			return bitReader.readBits8(count);
		}
	};
	struct ReadBit
	{
		MSBBitReader<ObsfuscatedInputStream> &bitReader;

		uint32_t operator()() const
		{
			return bitReader.readBits8(1);
		}
	};
	InitInputStream initInputStream{inputStream,bitReader};
	FinishStream finishStream{inputStream,_isObsfuscated,limitedDecompress};
	ReadBits readBits{bitReader};
	ReadBit readBit{bitReader};

	ForwardOutputStream outputStream{rawData,0,0};
	struct InitOutputStream
	{
		ForwardOutputStream &outputStream;

		void operator()(uint32_t start,uint32_t length) const
		{
			outputStream.reset(start,BoundsCheck::sum(start,length));
		}
	};
	InitOutputStream initOutputStream{outputStream};

	// fill unused tracks with zeros
	std::memset(rawData.data(),0,_rawSize);

	bool doInitContext{true};
	// quick: context used is 256 bytes
	// medium & deep: context used is 16384 bytes
	// heavy: context used is 4096/8192 bytes
	uint32_t contextLocation;
	std::unique_ptr<DynamicHuffmanDecoder<314>> deepDecoder;
	struct InitContext
	{
		bool &doInitContext;
		const uint32_t &contextBufferSize;
		OwnedBuffer &contextBuffer;
		uint32_t &contextLocation;
		std::unique_ptr<DynamicHuffmanDecoder<314>> &deepDecoder;

		void operator()() const
		{
			if (doInitContext)
			{
				if (contextBufferSize) std::memset(contextBuffer.data(),0,contextBufferSize);
				contextLocation=0;
				deepDecoder.reset();
				doInitContext=false;
			}
		}
	};
	InitContext initContext{doInitContext,_contextBufferSize,contextBuffer,contextLocation,deepDecoder};

	struct Checksum
	{
		uint16_t operator()(const ByteBuffer &src,uint32_t offset,uint32_t srcLength) const
		{
			uint16_t ret{0};
			for (uint32_t i=0;i<srcLength;i++) ret+=uint16_t(src[i+offset]);
			return ret;
		}
	};
	Checksum checksum;

	class BlockUnpacker
	{
	public:
		virtual ~BlockUnpacker() noexcept=default;
		virtual void operator()(ForwardOutputStream &output)=0;
	};

	struct UnpackNone : public BlockUnpacker
	{
		ObsfuscatedInputStream &inputStream;
		const uint32_t &limitedDecompress;

		UnpackNone(ObsfuscatedInputStream &_inputStream,const uint32_t &_limitedDecompress) :
			inputStream{_inputStream},
			limitedDecompress{_limitedDecompress}
		{
			// nothing needed
		}

		void operator()(ForwardOutputStream &output) final
		{
			for (uint32_t i=0;i<limitedDecompress&&!output.eof();i++)
				output.writeByte(inputStream.readByte());
		}
	};
	UnpackNone unpackNone{inputStream,limitedDecompress};

	// same as simple
	struct UnRLE
	{
		const uint32_t &limitedDecompress;

		void operator()(ForwardOutputStream &output,ObsfuscatedInputStream &input) const
		{
			while (!output.eof())
			{
				if (output.getOffset()>=limitedDecompress) return;
				uint8_t ch{input.readByte()};
				uint32_t count{1};
				if (ch==0x90U)
				{
					count=input.readByte();
					if (!count) count=1;
						else ch=input.readByte();
					if (count==0xffU)
					{
						count=uint32_t(input.readByte())<<8;
						count|=input.readByte();
					}
				}
				for (uint32_t i=0;i<count;i++) output.writeByte(ch);
			}
		}

		void operator()(ForwardOutputStream &output,ForwardInputStream &input) const
		{
			while (!output.eof())
			{
				if (output.getOffset()>=limitedDecompress) return;
				uint8_t ch{input.readByte()};
				uint32_t count{1};
				if (ch==0x90U)
				{
					count=input.readByte();
					if (!count) count=1;
						else ch=input.readByte();
					if (count==0xffU)
					{
						count=uint32_t(input.readByte())<<8;
						count|=input.readByte();
					}
				}
				for (uint32_t i=0;i<count;i++) output.writeByte(ch);
			}
		}
	};
	UnRLE unRLE{limitedDecompress};

	struct UnpackQuick : public BlockUnpacker
	{
		InitContext &initContext;
		const uint32_t &limitedDecompress;
		ReadBits &readBits;
		OwnedBuffer &contextBuffer;
		uint32_t &contextLocation;

		UnpackQuick(InitContext &_initContext,const uint32_t &_limitedDecompress,ReadBits &_readBits,OwnedBuffer &_contextBuffer,uint32_t &_contextLocation) :
			initContext{_initContext},
			limitedDecompress{_limitedDecompress},
			readBits{_readBits},
			contextBuffer{_contextBuffer},
			contextLocation{_contextLocation}
		{
			// nothing needed
		}

		void operator()(ForwardOutputStream &output) final
		{
			initContext();

			while (!output.eof())
			{
				if (output.getOffset()>=limitedDecompress) return;
				if (readBits(1))
				{
					output.writeByte(contextBuffer[contextLocation++]=readBits(8));
					contextLocation&=0xffU;
				} else {
					uint32_t count{readBits(2)+2};
					uint8_t offset{uint8_t(contextLocation-readBits(8)-1)};
					for (uint32_t i=0;i<count;i++)
					{
						output.writeByte(contextBuffer[contextLocation++]=contextBuffer[(i+offset)&0xffU]);
						contextLocation&=0xffU;
					}
				}
			}
			contextLocation+=5;
			contextLocation&=0xffU;
		}
	};
	UnpackQuick unpackQuick{initContext,limitedDecompress,readBits,contextBuffer,contextLocation};


	VariableLengthCodeDecoder lengthDecoder{7,7,8,8,8,9,9,9,9,10,10,10,11,11,11,12};

	struct UnpackMedium : public BlockUnpacker
	{
		struct DistanceBits
		{
			uint32_t count;
			ReadBits &readBits;

			uint32_t operator()(uint32_t bitCount) const
			{
				return (uint32_t(count&0xfU)<<(bitCount-4U))|readBits(bitCount-4U);
			}
		};

		InitContext &initContext;
		const uint32_t &limitedDecompress;
		ReadBits &readBits;
		OwnedBuffer &contextBuffer;
		uint32_t &contextLocation;
		const VariableLengthCodeDecoder<16> &lengthDecoder;

		UnpackMedium(InitContext &_initContext,const uint32_t &_limitedDecompress,ReadBits &_readBits,OwnedBuffer &_contextBuffer,uint32_t &_contextLocation,const VariableLengthCodeDecoder<16> &_lengthDecoder) :
			initContext{_initContext},
			limitedDecompress{_limitedDecompress},
			readBits{_readBits},
			contextBuffer{_contextBuffer},
			contextLocation{_contextLocation},
			lengthDecoder{_lengthDecoder}
		{
			// nothing needed
		}

		void operator()(ForwardOutputStream &output) final
		{
			initContext();

			while (!output.eof())
			{
				if (output.getOffset()>=limitedDecompress) return;
				if (readBits(1))
				{
					output.writeByte(contextBuffer[contextLocation++]=readBits(8));
					contextLocation&=0x3fffU;
				} else {
					uint32_t count{lengthDecoder.decode(readBits,readBits(4U))};
					DistanceBits distanceBits{count,readBits};
					uint32_t rawDist{lengthDecoder.decode(distanceBits,((count>>4U)&0xfU))};
					count=(count>>8U)+3U;

					uint32_t offset{contextLocation-rawDist-1U};
					for (uint32_t i=0;i<count;i++)
					{
						output.writeByte(contextBuffer[contextLocation++]=contextBuffer[(i+offset)&0x3fffU]);
						contextLocation&=0x3fffU;
					}
				}
			}
			contextLocation+=66;
			contextLocation&=0x3fffU;
		}
	};
	UnpackMedium unpackMedium{initContext,limitedDecompress,readBits,contextBuffer,contextLocation,lengthDecoder};

	struct UnpackDeep : public BlockUnpacker
	{
		InitContext &initContext;
		const uint32_t &limitedDecompress;
		ReadBit &readBit;
		ReadBits &readBits;
		OwnedBuffer &contextBuffer;
		uint32_t &contextLocation;
		std::unique_ptr<DynamicHuffmanDecoder<314>> &deepDecoder;
		const VariableLengthCodeDecoder<16> &lengthDecoder;

		UnpackDeep(InitContext &_initContext,const uint32_t &_limitedDecompress,ReadBit &_readBit,ReadBits &_readBits,OwnedBuffer &_contextBuffer,uint32_t &_contextLocation,std::unique_ptr<DynamicHuffmanDecoder<314>> &_deepDecoder,const VariableLengthCodeDecoder<16> &_lengthDecoder) :
			initContext{_initContext},
			limitedDecompress{_limitedDecompress},
			readBit{_readBit},
			readBits{_readBits},
			contextBuffer{_contextBuffer},
			contextLocation{_contextLocation},
			deepDecoder{_deepDecoder},
			lengthDecoder{_lengthDecoder}
		{
			// nothing needed
		}

		void operator()(ForwardOutputStream &output) final
		{
			initContext();
			if (!deepDecoder) deepDecoder=std::make_unique<DynamicHuffmanDecoder<314>>();

			while (!output.eof())
			{
				if (output.getOffset()>=limitedDecompress) return;
				uint32_t symbol=deepDecoder->decode(readBit);
				if (deepDecoder->getMaxFrequency()==0x8000U) deepDecoder->halve();
				deepDecoder->update(symbol);
				if (symbol<256)
				{
					output.writeByte(contextBuffer[contextLocation++]=symbol);
					contextLocation&=0x3fffU;
				} else {
					uint32_t count{symbol-253};	// minimum repeat is 3
					uint32_t offset{contextLocation-lengthDecoder.decode(readBits,readBits(4U))-1U};

					for (uint32_t i=0;i<count;i++)
					{
						output.writeByte(contextBuffer[contextLocation++]=contextBuffer[(i+offset)&0x3fffU]);
						contextLocation&=0x3fffU;
					}
				}
			}
			contextLocation+=60;
			contextLocation&=0x3fffU;
		}
	};
	UnpackDeep unpackDeep{initContext,limitedDecompress,readBit,readBits,contextBuffer,contextLocation,deepDecoder,lengthDecoder};

	// these are not part of the initContext like other methods
	std::unique_ptr<OptionalHuffmanDecoder<uint32_t>> symbolDecoder,offsetDecoder;
	bool heavyLastInitialized{false};		// this is part of initContext on some implementations. screwy!!!
	uint32_t heavyLastOffset;
	struct ReadTable
	{
		ReadBits &readBits;

		void operator()(std::unique_ptr<OptionalHuffmanDecoder<uint32_t>> &decoder,uint32_t countBits,uint32_t valueBits) const
		{
			decoder=std::make_unique<OptionalHuffmanDecoder<uint32_t>>();
			uint32_t count{readBits(countBits)};
			if (count)
			{
				std::array<uint8_t,512> lengthBuffer;
				// in order to speed up the deObsfuscation, do not send the hopeless
				// data into slow CreateOrderlyHuffmanTable
				uint64_t sum{0};
				for (uint32_t i=0;i<count;i++)
				{
					uint32_t bits{readBits(valueBits)};
					if (bits)
					{
						sum+=uint64_t(1U)<<(32-bits);
						if (sum>(uint64_t(1U)<<32))
							throw DecompressionError();
					}
					lengthBuffer[i]=bits;
				}
				decoder->createOrderlyHuffmanTable(lengthBuffer,count);
			} else {
				uint32_t index{readBits(countBits)};
				decoder->setEmpty(index);
			}
		}
	};
	ReadTable readTable{readBits};

	struct UnpackHeavy : public BlockUnpacker
	{
		InitContext &initContext;
		bool &heavyLastInitialized;
		uint32_t &heavyLastOffset;
		ReadTable &readTable;
		std::unique_ptr<OptionalHuffmanDecoder<uint32_t>> &symbolDecoder;
		std::unique_ptr<OptionalHuffmanDecoder<uint32_t>> &offsetDecoder;
		ReadBit &readBit;
		ReadBits &readBits;
		const uint32_t &limitedDecompress;
		OwnedBuffer &contextBuffer;
		uint32_t &contextLocation;
		bool initTables{false};
		bool use8kDict{false};

		UnpackHeavy(InitContext &_initContext,bool &_heavyLastInitialized,uint32_t &_heavyLastOffset,ReadTable &_readTable,std::unique_ptr<OptionalHuffmanDecoder<uint32_t>> &_symbolDecoder,std::unique_ptr<OptionalHuffmanDecoder<uint32_t>> &_offsetDecoder,ReadBit &_readBit,ReadBits &_readBits,const uint32_t &_limitedDecompress,OwnedBuffer &_contextBuffer,uint32_t &_contextLocation) :
			initContext{_initContext},
			heavyLastInitialized{_heavyLastInitialized},
			heavyLastOffset{_heavyLastOffset},
			readTable{_readTable},
			symbolDecoder{_symbolDecoder},
			offsetDecoder{_offsetDecoder},
			readBit{_readBit},
			readBits{_readBits},
			limitedDecompress{_limitedDecompress},
			contextBuffer{_contextBuffer},
			contextLocation{_contextLocation}
		{
			// nothing needed
		}

		void configure(bool _initTables,bool _use8kDict)
		{
			initTables=_initTables;
			use8kDict=_use8kDict;
		}

		void operator()(ForwardOutputStream &output) final
		{
			initContext();
			// well, this works. Why this works? dunno
			if (!heavyLastInitialized)
			{
				heavyLastOffset=use8kDict?0U:~0U;
				heavyLastInitialized=true;
			}

			if (initTables)
			{
				readTable(symbolDecoder,9,5);
				readTable(offsetDecoder,5,4);
			}

			uint32_t mask{use8kDict?0x1fffU:0xfffU};
			uint32_t bitLength{use8kDict?14U:13U};

			while (!output.eof())
			{
				if (output.getOffset()>=limitedDecompress) return;
				uint32_t symbol=symbolDecoder->decode(readBit);
				if (symbol<256)
				{
					output.writeByte(contextBuffer[contextLocation++]=symbol);
					contextLocation&=mask;
				} else {
					uint32_t count{symbol-253};	// minimum repeat is 3
					uint32_t offsetLength{offsetDecoder->decode(readBit)};
					uint32_t rawOffset{heavyLastOffset};
					if (offsetLength!=bitLength)
					{
						if (offsetLength) rawOffset=(1<<(offsetLength-1))|readBits(offsetLength-1);
							else rawOffset=0;
						heavyLastOffset=rawOffset;
					}
					uint32_t offset{contextLocation-rawOffset-1};
					for (uint32_t i=0;i<count;i++)
					{
						output.writeByte(contextBuffer[contextLocation++]=contextBuffer[(i+offset)&mask]);
						contextLocation&=mask;
					}
				}
			}
		}
	};
	UnpackHeavy unpackHeavy{initContext,heavyLastInitialized,heavyLastOffset,readTable,symbolDecoder,offsetDecoder,readBit,readBits,limitedDecompress,contextBuffer,contextLocation};

	struct HandleTrackSize
	{
		const uint32_t &limitedDecompress;
		const ForwardOutputStream &outputStream;

		void operator()() const
		{
			if (limitedDecompress!=~0U) return;
			if (!outputStream.eof() && outputStream.getOffset()&0x3ffU)
				throw DecompressionError();
		}
	};
	HandleTrackSize handleTrackSize{limitedDecompress,outputStream};

	struct UnpackRLE : public BlockUnpacker
	{
		UnRLE &unRLE;
		ObsfuscatedInputStream &inputStream;
		HandleTrackSize &handleTrackSize;

		UnpackRLE(UnRLE &_unRLE,ObsfuscatedInputStream &_inputStream,HandleTrackSize &_handleTrackSize) :
			unRLE{_unRLE},
			inputStream{_inputStream},
			handleTrackSize{_handleTrackSize}
		{
			// nothing needed
		}

		void operator()(ForwardOutputStream &output) final
		{
			unRLE(output,inputStream);
			handleTrackSize();
		}
	};
	UnpackRLE unpackRLE{unRLE,inputStream,handleTrackSize};

	bool _codeFixed{false};
	uint32_t trackLength=(_isHD)?22528:11264;
	for (uint32_t packedOffset=56,packedChunkLength=0;packedOffset!=_packedSize;packedOffset=BoundsCheck::sum(packedOffset,20U,packedChunkLength))
	{
		// There are some info tracks, at -1 or 80. ignore those (if still present)
		uint16_t trackNo{_packedData.readBE16(packedOffset+2)};
		packedChunkLength=_packedData.readBE16(packedOffset+6);
		if (trackNo==80) break;							// should not happen, this is already excluded
		// even though only -1 should be used I've seen -2 as well. ignore all negatives
		uint32_t tmpChunkLength{_packedData.readBE16(packedOffset+8)};		// after the first unpack (if twostage)
		uint32_t rawChunkLength{_packedData.readBE16(packedOffset+10)};		// after final unpack
		uint8_t flags{_packedData.read8(packedOffset+12)};
		uint8_t mode{_packedData.read8(packedOffset+13)};

		// could affect context, but in practice they are separate, even though there is no explicit reset
		// deal with decompression though...
		if (trackNo>=0x8000U)
		{
			initInputStream(packedOffset+20,packedChunkLength);
			finishStream();
			continue;
		}
		if (rawChunkLength>trackLength)
			throw DecompressionError();
		if (trackNo>80)
			throw DecompressionError();					// should not happen, already excluded

		uint32_t dataOffset{trackNo*trackLength};
		if (_rawOffset>dataOffset)
			throw DecompressionError();

		// this is screwy, but it is, what it is
		struct ProcessBlock
		{
			bool isObsfuscated;
			uint8_t mode;
			uint32_t packedOffset;
			uint32_t packedChunkLength;
			uint32_t tmpChunkLength;
			uint32_t rawChunkLength;
			uint32_t dataOffset;
			uint32_t rawOffset;
			const ByteBuffer &packedData;
			ByteBuffer &rawData;
			OwnedBuffer &tmpBuffer;
			ForwardOutputStream &outputStream;
			InitInputStream &initInputStream;
			FinishStream &finishStream;
			InitOutputStream &initOutputStream;
			UnRLE &unRLE;
			HandleTrackSize &handleTrackSize;
			Checksum &checksum;

			void applyFix() const
			{
				if (mode>=5U && !isObsfuscated)
				{
					uint32_t missingNo{uint32_t(outputStream.getEndOffset()-outputStream.getOffset())};
					uint16_t protoSum{checksum(rawData,dataOffset-rawOffset,rawChunkLength-missingNo)};
					uint16_t fileSum{packedData.readBE16(packedOffset+14)};

					// too many ways to interpret the data
					if (missingNo>1)
						throw DecompressionError();
					for (uint32_t i=0;i<missingNo;i++) outputStream.writeByte(0);

					if (protoSum!=fileSum)
					{
						// last byte can be trashed by the compressor, but fortunately it can be fixed
						protoSum-=*outputStream.history(1);
						uint16_t fixByte{uint16_t(fileSum-protoSum)};
						if (fixByte>=0x100U)
							throw DecompressionError();
						*outputStream.history(1)=uint8_t(fixByte);
					}
				} else handleTrackSize();
			}

			void operator()(bool doRLE,BlockUnpacker &func) const
			{
				initInputStream(packedOffset+20,packedChunkLength);
				if (doRLE)
				{
					ForwardOutputStream tmpOutputStream{tmpBuffer,0,tmpChunkLength};
					try
					{
						func(tmpOutputStream);
					} catch (const DecompressionError &) {
						// nothing needed
					}
					finishStream();
					uint32_t missingNo{static_cast<uint32_t>(tmpOutputStream.getEndOffset()-tmpOutputStream.getOffset())};
					ForwardInputStream tmpInputStream{tmpBuffer,0,tmpChunkLength-missingNo};
					initOutputStream(dataOffset-rawOffset,rawChunkLength);
					try
					{
						unRLE(outputStream,tmpInputStream);
					} catch (const DecompressionError &) {
						// nothing needed
					}
					applyFix();
				} else {
					initOutputStream(dataOffset-rawOffset,rawChunkLength);
					try
					{
						func(outputStream);
					} catch (const DecompressionError &) {
						// nothing needed
					}
					applyFix();
				}
				finishStream();
			}
		};
		ProcessBlock processBlock{_isObsfuscated,mode,packedOffset,packedChunkLength,tmpChunkLength,rawChunkLength,dataOffset,_rawOffset,_packedData,rawData,tmpBuffer,outputStream,initInputStream,finishStream,initOutputStream,unRLE,handleTrackSize,checksum};

		struct ProcessBlockCode
		{
			bool isObsfuscated;
			uint16_t trackNo;
			uint32_t minTrack;
			bool &codeFixed;
			ProcessBlock &processBlock;
			uint32_t &restartPosition;
			bool &doInitContext;
			ObsfuscatedInputStream &inputStream;
			uint32_t &limitedDecompress;
			ByteBuffer &rawData;
			Checksum &checksum;
			uint32_t dataOffset;
			uint32_t rawOffset;
			uint32_t rawChunkLength;
			const ByteBuffer &packedData;
			uint32_t packedOffset;

			void operator()(bool doRLE,BlockUnpacker &func) const
			{
				if (!isObsfuscated || trackNo!=minTrack || codeFixed)
				{
					processBlock(doRLE,func);
					return;
				}

				// fast try
				if (!trackNo && restartPosition<0x10000U) for (;restartPosition<0x10000U;restartPosition++)
				{
					try
					{
						doInitContext=true;
						inputStream.setCode(restartPosition);
						limitedDecompress=8;
						processBlock(doRLE,func);
						if ((rawData.readBE32(0)&0xffff'ff00U)!=FourCC("DOS\0")) continue;

						// now see if the candidate is any good
						doInitContext=true;
						inputStream.setCode(restartPosition);
						limitedDecompress=~0U;
						processBlock(doRLE,func);
						if (checksum(rawData,dataOffset-rawOffset,rawChunkLength)!=packedData.readBE16(packedOffset+14)) continue;
						codeFixed=true;
						return;
					} catch (const ByteBuffer::Error &) {
						// just continue
					} catch (const CodecDecoder::Error &) {
						// just continue
					}
				}

				// slow round
				limitedDecompress=~0U;
				for (;restartPosition<0x20000U;restartPosition++)
				{
					try
					{
						doInitContext=true;
						inputStream.setCode(restartPosition);
						processBlock(doRLE,func);
						if (checksum(rawData,dataOffset-rawOffset,rawChunkLength)!=packedData.readBE16(packedOffset+14)) continue;
						codeFixed=true;
						return;
					} catch (const ByteBuffer::Error &) {
						// just continue
					} catch (const CodecDecoder::Error &) {
						// just continue
					}
				}
				throw DecompressionError();
			}
		};
		ProcessBlockCode processBlockCode{_isObsfuscated,trackNo,_minTrack,_codeFixed,processBlock,restartPosition,doInitContext,inputStream,limitedDecompress,rawData,checksum,dataOffset,_rawOffset,rawChunkLength,_packedData,packedOffset};

		switch (mode)
		{
			case 0:
			processBlockCode(false,unpackNone);
			rawChunkLength=packedChunkLength;
			break;

			case 1:
			processBlockCode(false,unpackRLE);
			break;

			case 2:
			processBlockCode(true,unpackQuick);
			break;

			case 3:
			processBlockCode(true,unpackMedium);
			break;

			case 4:
			processBlockCode(true,unpackDeep);
			break;

			// heavy flags:
			// 2: (re-)initialize/read tables
			// 4: do RLE
			// heavy1 uses 4k dictionary (mode 5), whereas heavy2 uses 8k dictionary
			case 5:
			[[fallthrough]];
			case 6:
			unpackHeavy.configure(flags&2,mode==6);
			processBlockCode(flags&4,unpackHeavy);
			break;

			default:
			throw DecompressionError();
		}
		if (!(flags&1)) doInitContext=true;

		if (verify && checksum(rawData,dataOffset-_rawOffset,rawChunkLength)!=_packedData.readBE16(packedOffset+14))
			throw VerificationError();
	}
}

}
