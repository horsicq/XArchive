/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xssmdecoder.h"

#include "algo_utils.h"

#include <array>
#include <limits>

namespace {
const qint32 SSM_WINDOW_SIZE = 16384;
const qint32 SSM_WINDOW_MASK = SSM_WINDOW_SIZE - 1;
const qint32 SSM_OUTPUT_BUFFER_SIZE = 0x10000;

// PICTools assigns the 192 most useful byte values one-byte opcodes.  Values
// not present in this permutation are carried by the F0..FE literal-run
// commands.  The table is identical in methods 3 and 5.
constexpr std::array<quint8, 192> SSM_LITERALS = {{
    0x00, 0xff, 0x8b, 0x01, 0x45, 0x03, 0x80, 0xe8,
    0x02, 0x66, 0x89, 0x08, 0x26, 0x04, 0x67, 0x05,
    0x83, 0x77, 0x10, 0x50, 0xc3, 0xcd, 0x0a, 0x76,
    0x0f, 0xcb, 0x06, 0x90, 0x0c, 0xc0, 0x0b, 0xeb,
    0x85, 0xb7, 0xe9, 0xa1, 0x55, 0x47, 0xc7, 0x14,
    0x75, 0xf0, 0x74, 0x46, 0x09, 0x20, 0x18, 0x88,
    0x07, 0x4c, 0x56, 0xec, 0x8a, 0x8d, 0xc4, 0xc1,
    0x1c, 0x5e, 0x31, 0xe0, 0x0e, 0xb8, 0xf8, 0x3b,
    0x40, 0x3f, 0xfe, 0xd0, 0xc2, 0x72, 0x7d, 0x81,
    0x44, 0x52, 0x6a, 0xb6, 0x24, 0xe4, 0x57, 0xd8,
    0x1f, 0x0d, 0x5d, 0x1e, 0x8e, 0x5f, 0x16, 0xfc,
    0x34, 0xe5, 0x11, 0xa0, 0xd2, 0x30, 0xfa, 0x33,
    0x12, 0xc8, 0x8c, 0x65, 0x94, 0x2b, 0xc6, 0x7e,
    0x53, 0xdc, 0x32, 0x35, 0x1a, 0x64, 0x15, 0x19,
    0x7c, 0x38, 0x4e, 0xf4, 0xb0, 0x51, 0xd3, 0x73,
    0x5b, 0xa3, 0x29, 0x28, 0x4d, 0x3d, 0x82, 0x58,
    0xf6, 0xc9, 0xca, 0x36, 0x2c, 0x39, 0x78, 0xf7,
    0xe2, 0x84, 0xd1, 0x70, 0x5a, 0x13, 0x68, 0xd4,
    0xfb, 0x9a, 0x6c, 0xf3, 0xbd, 0x21, 0xcc, 0xb4,
    0x69, 0x3e, 0x2e, 0x60, 0x42, 0x3c, 0x98, 0x6e,
    0x1d, 0x54, 0xb3, 0xa4, 0xdb, 0x25, 0xb2, 0xa5,
    0x6f, 0x2d, 0xe1, 0x61, 0x49, 0x95, 0x5c, 0x59,
    0x6d, 0xea, 0xf2, 0x17, 0xd6, 0xbc, 0x4a, 0xa8,
    0xda, 0xef, 0x3a, 0x37, 0xfd, 0x22, 0x86, 0x2a
}};

constexpr std::array<quint8, 8> SSM_PREFIX_METHOD3 = {{
    0x4d, 0x5a, 0x80, 0x00, 0x01, 0x00, 0x00, 0x00
}};
constexpr std::array<quint8, 8> SSM_PREFIX_METHOD5 = {{
    0x4d, 0x5a, 0x90, 0x00, 0x03, 0x00, 0x00, 0x00
}};

enum class SSM_READ_RESULT {
    OK,
    END,
    IO_ERROR
};

class SSMReader
{
public:
    SSMReader(XBinary::DATAPROCESS_STATE *pState,
              XBinary::PDSTRUCT *pPdStruct)
        : m_pState(pState), m_pPdStruct(pPdStruct)
    {
    }

    SSM_READ_RESULT readByte(quint8 *pValue)
    {
        if (!m_pState || !pValue ||
            !XBinary::isPdStructNotCanceled(m_pPdStruct))
            return SSM_READ_RESULT::IO_ERROR;
        if ((m_pState->nInputLimit != -1) &&
            (m_pState->nCountInput >= m_pState->nInputLimit)) {
            return m_pState->nCountInput == m_pState->nInputLimit
                       ? SSM_READ_RESULT::END
                       : SSM_READ_RESULT::IO_ERROR;
        }

        char value = 0;
        const qint32 count = XBinary::_readDevice(&value, 1, m_pState);
        if (count == 1) {
            *pValue = quint8(value);
            return SSM_READ_RESULT::OK;
        }
        if ((count < 0) || m_pState->bReadError)
            return SSM_READ_RESULT::IO_ERROR;
        return SSM_READ_RESULT::END;
    }

private:
    XBinary::DATAPROCESS_STATE *m_pState = nullptr;
    XBinary::PDSTRUCT *m_pPdStruct = nullptr;
};

class SSMOutput
{
public:
    SSMOutput(XBinary::DATAPROCESS_STATE *pState,
              XBinary::PDSTRUCT *pPdStruct, qint64 nExpectedSize,
              qint64 nOutputLimit)
        : m_pState(pState),
          m_pPdStruct(pPdStruct),
          m_nExpectedSize(nExpectedSize),
          m_nOutputLimit(nOutputLimit)
    {
    }

    bool canEmit(qint32 nSize) const
    {
        if ((nSize < 0) ||
            (m_nProduced >
             (std::numeric_limits<qint64>::max)() - nSize))
            return false;
        const qint64 end = m_nProduced + nSize;
        return (end <= m_nExpectedSize) &&
               ((m_nOutputLimit < 0) || (end <= m_nOutputLimit));
    }

    bool writeByte(quint8 value)
    {
        if (!canEmit(1) ||
            !XBinary::isPdStructNotCanceled(m_pPdStruct))
            return false;
        m_buffer[m_nBuffered++] = char(value);
        ++m_nProduced;
        return (m_nBuffered < SSM_OUTPUT_BUFFER_SIZE) || flush();
    }

    bool flush()
    {
        if (!m_nBuffered) return true;
        if (!XBinary::isPdStructNotCanceled(m_pPdStruct) ||
            (XBinary::_writeDevice(m_buffer.data(), m_nBuffered,
                                   m_pState) != m_nBuffered))
            return false;
        m_nBuffered = 0;
        return true;
    }

    qint64 produced() const { return m_nProduced; }

private:
    XBinary::DATAPROCESS_STATE *m_pState = nullptr;
    XBinary::PDSTRUCT *m_pPdStruct = nullptr;
    qint64 m_nExpectedSize = 0;
    qint64 m_nOutputLimit = -1;
    std::array<char, SSM_OUTPUT_BUFFER_SIZE> m_buffer = {};
    qint32 m_nBuffered = 0;
    qint64 m_nProduced = 0;
};

class SSMHistoryOutput
{
public:
    explicit SSMHistoryOutput(SSMOutput *pOutput) : m_pOutput(pOutput)
    {
    }

    bool emitByte(quint8 value)
    {
        if (!m_pOutput || !m_pOutput->writeByte(value)) return false;
        m_history[m_nHistoryPosition] = value;
        m_nHistoryPosition =
            (m_nHistoryPosition + 1) & SSM_WINDOW_MASK;
        return true;
    }

    bool emitMatch(qint32 distance, qint32 length)
    {
        if (!m_pOutput || (distance < 1) ||
            (distance > SSM_WINDOW_SIZE) ||
            (distance > m_pOutput->produced()) ||
            !m_pOutput->canEmit(length)) {
            return false;
        }
        for (qint32 i = 0; i < length; ++i) {
            const qint32 source =
                (m_nHistoryPosition + SSM_WINDOW_SIZE - distance) &
                SSM_WINDOW_MASK;
            if (!emitByte(m_history[source])) return false;
        }
        return true;
    }

private:
    SSMOutput *m_pOutput = nullptr;
    std::array<quint8, SSM_WINDOW_SIZE> m_history = {};
    qint32 m_nHistoryPosition = 0;
};

bool readRequired(SSMReader *pReader, quint8 *pValue,
                  bool *pDataError, bool *pIOError)
{
    if (!pReader || !pValue || !pDataError || !pIOError) return false;
    const SSM_READ_RESULT result = pReader->readByte(pValue);
    if (result == SSM_READ_RESULT::OK) return true;
    if (result == SSM_READ_RESULT::END)
        *pDataError = true;
    else
        *pIOError = true;
    return false;
}
}  // namespace

bool XSSMDecoder::decompress(XBinary::DATAPROCESS_STATE *pState,
                             qint32 nMethod,
                             XBinary::PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pDeviceInput || !pState->pDeviceOutput ||
        (pState->nInputOffset < 0) || (pState->nInputLimit < -1) ||
        ((nMethod != 3) && (nMethod != 5)))
        return false;

    bool converted = false;
    const qint64 expectedSize =
        pState->mapProperties
            .value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE)
            .toLongLong(&converted);
    if (!converted || (expectedSize < 8)) return false;

    qint64 outputLimit = -1;
    if (!XBinary::getUnpackOutputLimit(pState->mapUnpackProperties,
                                       &outputLimit) ||
        ((outputLimit >= 0) && (expectedSize > outputLimit)) ||
        !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                            expectedSize))
        return false;

    Algo_utils::prepareState(pState);
    if (pState->bReadError || pState->bWriteError ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    const qint64 maximum = (std::numeric_limits<qint64>::max)();
    if ((pState->nProcessedOffset < 0) ||
        (pState->nProcessedLimit < -1) ||
        ((pState->nProcessedLimit != -1) &&
         (pState->nProcessedOffset > maximum - pState->nProcessedLimit))) {
        pState->bWriteError = true;
        return false;
    }

    SSMReader reader(pState, pPdStruct);
    SSMOutput output(pState, pPdStruct, expectedSize, outputLimit);
    SSMHistoryOutput historyOutput(&output);
    bool dataError = false;
    bool ioError = false;
    bool cleanEnd = false;

    const std::array<quint8, 8> &prefix =
        nMethod == 3 ? SSM_PREFIX_METHOD3 : SSM_PREFIX_METHOD5;
    for (quint8 value : prefix) {
        if (!historyOutput.emitByte(value)) {
            ioError = pState->bWriteError ||
                      !XBinary::isPdStructNotCanceled(pPdStruct);
            dataError = !ioError;
            break;
        }
    }

    while (!cleanEnd && !dataError && !ioError &&
           XBinary::isPdStructNotCanceled(pPdStruct)) {
        quint8 command = 0;
        const SSM_READ_RESULT commandResult = reader.readByte(&command);
        if (commandResult == SSM_READ_RESULT::END) {
            dataError = true;  // Every stream has an FF FF terminator.
            break;
        }
        if (commandResult != SSM_READ_RESULT::OK) {
            ioError = true;
            break;
        }

        if (command < 0xc0U) {
            if (!historyOutput.emitByte(SSM_LITERALS[command])) {
                ioError = pState->bWriteError ||
                          !XBinary::isPdStructNotCanceled(pPdStruct);
                dataError = !ioError;
            }
        } else if (command < 0xe0U) {
            quint8 lowDistance = 0;
            if (!readRequired(&reader, &lowDistance, &dataError,
                              &ioError))
                continue;
            const qint32 lengthBase = command < 0xd0U ? 3 : 7;
            const qint32 length =
                lengthBase + ((command >> 2) & 3U);
            const qint32 distance =
                ((qint32(command & 3U) << 8) | lowDistance) + 1;
            if (!historyOutput.emitMatch(distance, length)) {
                ioError = pState->bWriteError ||
                          !XBinary::isPdStructNotCanceled(pPdStruct);
                dataError = !ioError;
            }
        } else if (command < 0xf0U) {
            quint8 high = 0;
            quint8 low = 0;
            if (!readRequired(&reader, &high, &dataError, &ioError) ||
                !readRequired(&reader, &low, &dataError, &ioError))
                continue;
            const quint16 value =
                (quint16(high) << 8) | quint16(low);
            const qint32 length =
                4 + 4 * qint32(command & 0x0fU) + (value >> 14);
            const qint32 distance = (value & 0x3fffU) + 1;
            if (!historyOutput.emitMatch(distance, length)) {
                ioError = pState->bWriteError ||
                          !XBinary::isPdStructNotCanceled(pPdStruct);
                dataError = !ioError;
            }
        } else if (command < 0xffU) {
            const qint32 length = (command & 0x0fU) + 1;
            if (!output.canEmit(length)) {
                dataError = true;
                continue;
            }
            for (qint32 i = 0; i < length; ++i) {
                quint8 value = 0;
                if (!readRequired(&reader, &value, &dataError,
                                  &ioError))
                    break;
                if (!historyOutput.emitByte(value)) {
                    ioError = pState->bWriteError ||
                              !XBinary::isPdStructNotCanceled(pPdStruct);
                    dataError = !ioError;
                    break;
                }
            }
        } else {
            quint8 marker = 0;
            if (!readRequired(&reader, &marker, &dataError, &ioError))
                continue;
            if ((marker != 0xffU) ||
                (output.produced() != expectedSize)) {
                dataError = true;
                continue;
            }

            quint8 trailing = 0;
            const SSM_READ_RESULT tailResult = reader.readByte(&trailing);
            if (tailResult == SSM_READ_RESULT::END)
                cleanEnd = true;
            else if (tailResult == SSM_READ_RESULT::IO_ERROR)
                ioError = true;
            else
                dataError = true;
        }
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) ioError = true;
    bool result = cleanEnd && !dataError && !ioError &&
                  (output.produced() == expectedSize) &&
                  !pState->bReadError && !pState->bWriteError;
    if (result && !output.flush()) result = false;
    return result && !pState->bReadError && !pState->bWriteError &&
           XBinary::isPdStructNotCanceled(pPdStruct);
}
