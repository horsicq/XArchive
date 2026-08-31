/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XDEARKDECODER_H
#define XDEARKDECODER_H

#include <QString>

// Project-owned, bounded entry point for the private legacy-codec engine.
// The engine can only write a ZIP at the caller-selected path and can only
// dispatch modules explicitly supplied by XArchive.
class XDearkDecoder final {
public:
    struct LIMITS {
        qint64 maxFiles = 0;
        qint64 maxFileSize = 0;
        qint64 maxTotalSize = 0;
    };

    struct RESULT {
        bool runResult = false;
        bool fatalError = false;
        qint32 errorCount = 0;
        qint32 extractedFiles = 0;
        qint64 totalOutputSize = 0;
        QString module;
        QString errorMessage;
    };

    static bool extractToZip(const QString &inputPath,
                             const QString &outputPath,
                             const LIMITS &limits, RESULT *result);
    static bool isSupportedModule(const QString &module);

private:
    XDearkDecoder() = delete;
};

#endif  // XDEARKDECODER_H
