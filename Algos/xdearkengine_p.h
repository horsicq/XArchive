/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XDEARKENGINE_P_H
#define XDEARKENGINE_P_H

#include <QString>
#include <QStringList>

class XDearkEngine final
{
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

    static QStringList supportedModules();
    static bool extractToZip(const QString &inputPath,
                             const QString &outputPath,
                             const LIMITS &limits, RESULT *result);

private:
    XDearkEngine() = delete;
};

#endif  // XDEARKENGINE_P_H
