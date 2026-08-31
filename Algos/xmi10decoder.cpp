/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xmi10decoder.h"

#include "algo_utils.h"

#include <QByteArray>

#include <limits>

namespace
{
const qint64 MI10_MAX_PACKED_SIZE = 512LL * 1024LL * 1024LL;

qint32 mi10ReadBackward(const QByteArray &input, qint64 *pInputPosition)
{
    // Bytes 0 and 1 are the fixed zero marker and per-block escape byte.
    if (!pInputPosition || (*pInputPosition <= 2) ||
        (*pInputPosition > input.size())) {
        return -1;
    }
    --(*pInputPosition);
    return static_cast<quint8>(input.at(*pInputPosition));
}
}

XMI10Decoder::XMI10Decoder(QObject *pParent) : QObject(pParent)
{
}

bool XMI10Decoder::decompress(XBinary::DATAPROCESS_STATE *pState,
                              XBinary::PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pDeviceInput || !pState->pDeviceOutput ||
        (pState->nInputOffset < 0) || (pState->nInputLimit < 3) ||
        (pState->nInputLimit > MI10_MAX_PACKED_SIZE) ||
        (pState->nInputLimit > (std::numeric_limits<qint32>::max)()) ||
        !pState->mapProperties.contains(
            XBinary::FPART_PROP_UNCOMPRESSEDSIZE))
        return false;

    bool bSizeOK = false;
    const qint64 nOutputSize = pState->mapProperties.value(
        XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bSizeOK);
    if (!bSizeOK || (nOutputSize <= 0) ||
        (nOutputSize > (std::numeric_limits<qint32>::max)()) ||
        !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                            nOutputSize) ||
        (pState->nInputLimit >
         (std::numeric_limits<qint64>::max)() - nOutputSize))
        return false;

    // MI10 is decoded in-place from the end on its original platform.  Keep
    // both finite spans in memory here so every backward source reference can
    // be checked before publishing any output.
    XBinary::UNPACK_MEMORY_RESERVATION reservation;
    if (!reservation.acquire(pState->mapUnpackProperties,
                             pState->nInputLimit + nOutputSize))
        return false;

    Algo_utils::prepareState(pState);
    if (pState->bReadError || pState->bWriteError ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    QByteArray input(static_cast<qint32>(pState->nInputLimit), 0);
    qint64 nReadTotal = 0;
    while ((nReadTotal < pState->nInputLimit) &&
           XBinary::isPdStructNotCanceled(pPdStruct))
    {
        const qint32 nChunk = static_cast<qint32>(qMin<qint64>(
            1024 * 1024, pState->nInputLimit - nReadTotal));
        const qint32 nRead = XBinary::_readDevice(
            input.data() + nReadTotal, nChunk, pState);
        if (nRead != nChunk) return false;
        nReadTotal += nRead;
    }
    if ((nReadTotal != pState->nInputLimit) || pState->bReadError ||
        !XBinary::isPdStructNotCanceled(pPdStruct) ||
        (static_cast<quint8>(input.at(0)) != 0))
        return false;

    QByteArray output(static_cast<qint32>(nOutputSize), 0);
    const quint8 nEscape = static_cast<quint8>(input.at(1));
    qint64 nInputPosition = input.size();
    qint64 nOutputPosition = nOutputSize;

    while ((nOutputPosition > 0) &&
           XBinary::isPdStructNotCanceled(pPdStruct))
    {
        const qint32 nValue = mi10ReadBackward(input, &nInputPosition);
        if (nValue < 0) return false;
        if (nValue != nEscape)
        {
            --nOutputPosition;
            output[static_cast<qint32>(nOutputPosition)] =
                static_cast<char>(nValue);
            continue;
        }

        const qint32 nControl = mi10ReadBackward(input, &nInputPosition);
        if (nControl < 0) return false;
        if (nControl == 0)
        {
            // ESC,00 represents one literal occurrence of the escape byte.
            --nOutputPosition;
            output[static_cast<qint32>(nOutputPosition)] =
                static_cast<char>(nEscape);
            continue;
        }

        qint64 nDistance = nControl;
        qint64 nLength = 3;
        if (nControl >= 0x80)
        {
            const qint32 nLengthDistance =
                mi10ReadBackward(input, &nInputPosition);
            if (nLengthDistance < 0) return false;
            nDistance =
                ((static_cast<qint64>(nControl & 0x7f) << 4) |
                 (nLengthDistance & 0x0f)) + 1;
            if (nLengthDistance < 0x80)
            {
                nLength = (nLengthDistance >> 4) + 4;
            }
            else
            {
                const qint32 nLengthLow =
                    mi10ReadBackward(input, &nInputPosition);
                if (nLengthLow < 0) return false;
                nLength =
                    ((static_cast<qint64>(nLengthDistance & 0x70) << 4) |
                     nLengthLow) + 12;
            }
        }

        if ((nLength <= 0) || (nLength > nOutputPosition) ||
            (nDistance <= 0))
            return false;
        for (qint64 i = 0; i < nLength; ++i)
        {
            --nOutputPosition;
            if (nDistance >= (nOutputSize - nOutputPosition)) return false;
            output[static_cast<qint32>(nOutputPosition)] = output.at(
                static_cast<qint32>(nOutputPosition + nDistance));
        }
    }

    if ((nOutputPosition != 0) || (nInputPosition != 2) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    if (pState->mapProperties.value(XBinary::FPART_PROP_CHECKSUMTYPE)
            .toString() == QLatin1String("MI10 byte sum"))
    {
        bool bChecksumOK = false;
        const quint32 nExpectedChecksum = pState->mapProperties.value(
            XBinary::FPART_PROP_CHECKSUM).toString().toUInt(&bChecksumOK, 16);
        quint32 nActualChecksum = 0;
        for (char c : output)
            nActualChecksum += static_cast<quint8>(c);
        if (!bChecksumOK || (nActualChecksum != nExpectedChecksum))
            return false;
    }

    const qint32 nWritten = XBinary::_writeDevice(
        output.constData(), output.size(), pState);
    return (nWritten == output.size()) && !pState->bReadError &&
           !pState->bWriteError &&
           (pState->nCountInput == pState->nInputLimit) &&
           (pState->nCountOutput == nOutputSize) &&
           XBinary::isPdStructNotCanceled(pPdStruct);
}
