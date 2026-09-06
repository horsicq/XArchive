/* Copyright (c) 2026 hors<horsicq@gmail.com>
 * MIT License
 */
#ifndef XU3CONSOLE_H
#define XU3CONSOLE_H

#include "xarchiveconsole.h"

// Opt-in U3 command adapter. Native and 7-Zip parsing remain untouched.
class XU3Console {
public:
    enum COMMAND { HELP, TYPE, SORT, LIST, TEST, TESTLIST, TESTSORT, EXTRACT, EXTRACT_PATHS };
    struct OPTIONS {
        COMMAND command = HELP;
        QString source;
        QString destination;
        bool rename = false;
        bool valid = true;
    };
    struct TARGET { QString file; QString relativeDirectory; };
    struct PATHS {
        QString sourceDirectory;
        QString mask;
        QString destination;
        bool pattern = false;
        QList<TARGET> targets;
    };

    static bool process(XArchiveConsole &console, const QStringList &arguments, qint32 *result);
    static OPTIONS parse(const QStringList &operands);
    static bool prepare(const OPTIONS &options, PATHS *paths, QString *error);
    static QString batchDestination(const OPTIONS &options, const PATHS &paths, const TARGET &target);
    static QString help();

private:
    static bool commandMatches(const QString &token, const char *alias);
    static bool collect(const QString &directory, const QString &relativeDirectory, const QString &mask, const QString &excludedDirectory,
                        QList<TARGET> *targets, QSet<QString> *visited, int depth, QString *error);
    static XOptions::CR run(XArchiveConsole &console, const OPTIONS &options, const PATHS &paths);
    static XOptions::CR sortFile(const QString &source, const QString &destination, XBinary::FT type);
};

#endif
