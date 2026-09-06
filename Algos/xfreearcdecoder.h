/* Copyright (c) 2026 hors<horsicq@gmail.com>
 * MIT License
 */
#ifndef XFREEARCDECODER_H
#define XFREEARCDECODER_H

#include "xbinary.h"

// Bounded native decoder for the observed FreeArc method-chain subset.
// The caller authenticates container/member checksums and publishes output.
class XFreeArcDecoder {
public:
    static bool supports(const QByteArray &method);
    // Conservative peak byte-buffer/codec workspace estimate, including the
    // retained caller input capacity. Returns -1 for unsupported parameters.
    // A composite chain may require a stage cap larger than its final output.
    static qint64 memoryRequired(const QByteArray &method, qint64 inputCapacity,
                                 qint64 maxStageBytes, XBinary::PDSTRUCT *pd = nullptr);
    static bool decode(const QByteArray &input, const QByteArray &method,
                       qint64 expectedSize, qint64 maxBytes, QByteArray *output,
                       XBinary::PDSTRUCT *pd = nullptr);
};

#endif
