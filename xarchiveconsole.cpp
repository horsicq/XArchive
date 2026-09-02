/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xarchiveconsole.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaType>
#include <QSet>
#include <QVector>

#include <algorithm>
#include <cstdio>

XArchiveConsole::COMMAND::COMMAND()
    : verb(VERB_NONE),
      dialect(DIALECT_NATIVE),
      listFormat(LISTFORMAT_NATIVE),
      resultFormat(RESULTFORMAT_TEXT),
      overwrite(OVERWRITE_ALWAYS),
      fileType(XBinary::FT_UNKNOWN),
      bFlatten(false),
      bVerbose(false),
      bQuiet(false),
      bIgnoreCase(false)
{
}

XArchiveConsole::XArchiveConsole(QObject *pParent)
    : QObject(pParent),
      // ---- operations (POSIX tar letters where they exist) ----
      m_clList(QStringList() << "t" << "l" << "list" << "listarchive" << "showarchive", "List archive contents."),
      m_clExtract(QStringList() << "x" << "extract", "Extract archive members."),
      m_clExtractTo(QStringList() << "extractarchive", "Extract all archive entries to <directory>.", "directory"),
      m_clVerify(QStringList() << "W" << "verify" << "test" << "testarchive", "Verify every archive member without writing files."),
      m_clToStdout(QStringList() << "O" << "to-stdout" << "stdout", "Write the selected members to standard output."),
      m_clInfo(QStringList() << "i" << "info", "Show file information."),
      m_clEntropy(QStringList() << "e" << "entropy", "Show file entropy."),
      m_clStruct(QStringList() << "s" << "struct", "Show one named structure, e.g. 'Hash' or 'Hash#MD5'.", "name"),
      m_clStructs(QStringList() << "S" << "structs" << "showstructs", "Show every available structure."),
      m_clFormats(QStringList() << "formats" << "listformats", "List the container formats this build can open."),
      // ---- modifiers ----
      m_clDirectory(QStringList() << "C" << "directory", "Extract into <directory>.", "directory"),
      m_clFile(QStringList() << "f" << "file", "Archive to operate on; repeatable, and operands work too.", "file"),
      m_clExclude(QStringList() << "X" << "exclude", "Skip members matching <pattern>; repeatable.", "pattern"),
      m_clInclude(QStringList() << "include", "Keep only members matching <pattern>; repeatable. Operands do the same.", "pattern"),
      m_clKeep(QStringList() << "k" << "keep-old-files", "Keep existing destination files instead of replacing them."),
      m_clOverwrite(QStringList() << "overwrite", "Existing destination files: always (default), skip, or rename.", "mode", "always"),
      m_clFlatten(QStringList() << "j" << "flatten", "Drop stored directory components when extracting."),
      m_clIgnoreCase(QStringList() << "I" << "ignore-case", "Match member patterns case-insensitively."),
      m_clPassword(QStringList() << "P" << "password", "Archive password.", "password"),
      m_clPasswordStdin(QStringList() << "password-stdin", "Read the archive password as one UTF-8 line from standard input."),
      m_clPasswordHex(QStringList() << "H" << "password-hex", "Exact legacy archive password bytes as hexadecimal.", "hex"),
      m_clCodePage(QStringList() << "codepage", "Windows code page for legacy archive filenames and password bytes.", "number"),
      m_clProbeTimeout(QStringList() << "probe-timeout", "Maximum automatic archive-probe time per target in milliseconds (0 disables).", "milliseconds", "20000"),
      m_clStopOnError(QStringList() << "stop-on-error" << "stoponerror", "Abort extraction and roll the destination back when a member fails."),
      m_clFileType(QStringList() << "F" << "filetype", "Force the container type (e.g. PE, ELF, ZIP).", "type"),
      m_clFormat(QStringList() << "o" << "format",
                 "Output layout: native, technical, unzip, unzip-verbose, zipinfo, json, xml, csv, tsv, or text.", "layout"),
      m_clVerbose(QStringList() << "b" << "verbose", "Show verbose output with detailed information."),
      m_clQuiet(QStringList() << "q" << "quiet", "Suppress progress and summary lines."),
      m_clNoColor(QStringList() << "N" << "no-color" << "nocolor", "Disable colour output."),
      // ---- pre-POSIX output switches, kept as long-only aliases of -o ----
      m_clAsXml(QStringList() << "xml", "Output results in XML format."),
      m_clAsJson(QStringList() << "json", "Output results in JSON format."),
      m_clAsCsv(QStringList() << "csv", "Output results in CSV format."),
      m_clAsTsv(QStringList() << "tsv", "Output results in TSV format."),
      m_clAsPlainText(QStringList() << "plaintext", "Output results as plain text."),
      m_bEmbedded(false),
      m_nProbeTimeout(20000),
      m_bProbeTimeoutOccurred(false)
{
    // Best-effort extraction is the console default (like mainstream archive
    // tools): damaged or partially present archives yield every recoverable
    // member, and the skip tally is reported after the run.  --stop-on-error
    // restores the strict all-or-nothing transaction.
    m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_CONTINUEONERROR, true);
}

bool XArchiveConsole::addOptions(QCommandLineParser *pParser)
{
    if (m_bEmbedded) {
        return addEmbeddedOptions(pParser);
    }

    bool bAllRegistered = true;

    bAllRegistered = addOptionChecked(pParser, m_clList) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clExtract) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clExtractTo) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clVerify) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clToStdout) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clInfo) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clEntropy) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clStruct) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clStructs) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clFormats) && bAllRegistered;

    bAllRegistered = addOptionChecked(pParser, m_clDirectory) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clFile) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clExclude) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clInclude) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clKeep) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clOverwrite) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clFlatten) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clIgnoreCase) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clPassword) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clPasswordStdin) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clPasswordHex) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clCodePage) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clProbeTimeout) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clStopOnError) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clFileType) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clFormat) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clVerbose) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clQuiet) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clNoColor) && bAllRegistered;

    bAllRegistered = addOptionChecked(pParser, m_clAsXml) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clAsJson) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clAsCsv) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clAsTsv) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clAsPlainText) && bAllRegistered;

    return bAllRegistered;
}

// QCommandLineParser::addOption() returns false and drops the option when a
// name is already taken, which loses a switch silently -- that is how -v was
// lost to addVersionOption() and --verbose stopped existing.  Refuse to run
// with a half-registered command line instead.
bool XArchiveConsole::addOptionChecked(QCommandLineParser *pParser, const QCommandLineOption &option)
{
    if (pParser->addOption(option)) {
        return true;
    }

    printf("Internal error: duplicate command line option '%s'\n", option.names().join(QChar('/')).toUtf8().data());

    return false;
}

void XArchiveConsole::setEmbedded(bool bEmbedded)
{
    m_bEmbedded = bEmbedded;
}

bool XArchiveConsole::isEmbedded() const
{
    return m_bEmbedded;
}

QCommandLineOption XArchiveConsole::longOnly(const QCommandLineOption &option)
{
    QStringList listNames;
    const QStringList listOriginal = option.names();

    for (const QString &sName : listOriginal) {
        if (sName.length() > 1) {
            listNames.append(sName);
        }
    }

    return QCommandLineOption(listNames, option.description(), option.valueName(), option.defaultValues().value(0));
}

// Only the archive commands, long spellings only.  The host owns every letter
// and every viewer/output switch; registering ours would either lose the
// option (QCommandLineParser drops an option whose name is taken) or steal a
// letter that already means something else there.
bool XArchiveConsole::addEmbeddedOptions(QCommandLineParser *pParser)
{
    bool bAllRegistered = true;

    bAllRegistered = addOptionChecked(pParser, longOnly(m_clList)) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, longOnly(m_clExtract)) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, longOnly(m_clExtractTo)) && bAllRegistered;

    // "--test" is diec's signature-test directory switch, so the verify
    // command keeps only the spellings that cannot collide.
    bAllRegistered =
        addOptionChecked(pParser, QCommandLineOption(QStringList() << "verify" << "testarchive", m_clVerify.description())) && bAllRegistered;

    bAllRegistered = addOptionChecked(pParser, longOnly(m_clToStdout)) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, longOnly(m_clFormats)) && bAllRegistered;

    bAllRegistered = addOptionChecked(pParser, longOnly(m_clDirectory)) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, longOnly(m_clExclude)) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, longOnly(m_clInclude)) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, longOnly(m_clKeep)) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, longOnly(m_clOverwrite)) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, longOnly(m_clFlatten)) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, longOnly(m_clIgnoreCase)) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, longOnly(m_clPassword)) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, longOnly(m_clPasswordStdin)) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, longOnly(m_clPasswordHex)) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, longOnly(m_clCodePage)) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, longOnly(m_clProbeTimeout)) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, longOnly(m_clStopOnError)) && bAllRegistered;

    return bAllRegistered;
}

bool XArchiveConsole::processModes(const QCommandLineParser *pParser, const QStringList &listArgs, XBinary::FT fileType, bool bVerbose, qint32 *pnResult)
{
    COMMAND command;
    command.dialect = DIALECT_NATIVE;
    command.listTargets = listArgs;
    command.fileType = fileType;
    command.bVerbose = bVerbose;
    command.listFormat = bVerbose ? LISTFORMAT_TECHNICAL : LISTFORMAT_NATIVE;
    command.bFlatten = pParser->isSet(longOnly(m_clFlatten));
    command.bIgnoreCase = pParser->isSet(longOnly(m_clIgnoreCase));
    command.listIncludes = pParser->values(longOnly(m_clInclude));
    command.listExcludes = pParser->values(longOnly(m_clExclude));

    if (pParser->isSet(longOnly(m_clDirectory))) {
        command.sOutputDirectory = pParser->value(longOnly(m_clDirectory));
    }

    if (pParser->isSet(longOnly(m_clExtractTo))) {
        command.sOutputDirectory = pParser->value(longOnly(m_clExtractTo));
    }

    const QString sOverwrite = pParser->value(longOnly(m_clOverwrite));

    if (sOverwrite == QLatin1String("skip")) command.overwrite = OVERWRITE_SKIP;
    else if (sOverwrite == QLatin1String("rename")) command.overwrite = OVERWRITE_RENAME;

    if (pParser->isSet(longOnly(m_clKeep))) {
        command.overwrite = OVERWRITE_SKIP;
    }

    const QCommandLineOption clVerify(QStringList() << "verify" << "testarchive", m_clVerify.description());

    bool bProcessed = false;

    if (pParser->isSet(longOnly(m_clFormats))) {
        command.verb = VERB_FORMATS;
        const XOptions::CR cr = execute(command);
        if ((cr != XOptions::CR_SUCCESS) && pnResult) *pnResult = cr;
        bProcessed = true;
    }

    if (pParser->isSet(longOnly(m_clList))) {
        command.verb = VERB_LIST;
        const XOptions::CR cr = execute(command);
        if ((cr != XOptions::CR_SUCCESS) && pnResult) *pnResult = cr;
        bProcessed = true;
    }

    if (pParser->isSet(clVerify)) {
        command.verb = VERB_TEST;
        const XOptions::CR cr = execute(command);
        if ((cr != XOptions::CR_SUCCESS) && pnResult) *pnResult = cr;
        bProcessed = true;
    }

    if (pParser->isSet(longOnly(m_clToStdout))) {
        command.verb = VERB_STDOUT;
        const XOptions::CR cr = execute(command);
        if ((cr != XOptions::CR_SUCCESS) && pnResult) *pnResult = cr;
        bProcessed = true;
    }

    if (pParser->isSet(longOnly(m_clExtract)) || pParser->isSet(longOnly(m_clExtractTo))) {
        command.verb = VERB_EXTRACT;
        const XOptions::CR cr = execute(command);
        if ((cr != XOptions::CR_SUCCESS) && pnResult) *pnResult = cr;
        bProcessed = true;
    }

    return bProcessed;
}

bool XArchiveConsole::applyOptions(const QCommandLineParser *pParser, XOptions::CR *pcrResult)
{
    bool bProbeTimeoutValid = false;
    const qint64 nProbeTimeoutMs = pParser->value(m_clProbeTimeout).toLongLong(&bProbeTimeoutValid);

    if (!bProbeTimeoutValid || (nProbeTimeoutMs < 0)) {
        printf("Error: --probe-timeout requires a non-negative number of milliseconds\n");
        if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
        return false;
    }

    m_nProbeTimeout = nProbeTimeoutMs;

    m_mapUnpackProperties.clear();

    if (!pParser->isSet(m_clStopOnError)) {
        m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_CONTINUEONERROR, true);
    }

    QString sArchivePassword;
    const qint32 nPasswordOptions = qint32(pParser->isSet(m_clPassword)) + qint32(pParser->isSet(m_clPasswordStdin)) + qint32(pParser->isSet(m_clPasswordHex));

    if (nPasswordOptions > 1) {
        printf("Error: use only one of -P/--password, --password-stdin, or -H/--password-hex\n");
        if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
        return false;
    }

    if (pParser->isSet(m_clPasswordStdin)) {
        QFile passwordInput;

        if (!passwordInput.open(stdin, QIODevice::ReadOnly, QFileDevice::DontCloseHandle)) {
            printf("Error: cannot read archive password from standard input\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }

        QByteArray baPassword = passwordInput.readLine(1024 * 1024);

        if (!baPassword.endsWith('\n') && !passwordInput.atEnd()) {
            printf("Error: archive password is too long\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }

        if (baPassword.endsWith('\n')) baPassword.chop(1);
        if (baPassword.endsWith('\r')) baPassword.chop(1);

        sArchivePassword = QString::fromUtf8(baPassword);
    } else if (pParser->isSet(m_clPassword)) {
        sArchivePassword = pParser->value(m_clPassword);
    }

    if (!sArchivePassword.isEmpty()) {
        m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_PASSWORD, sArchivePassword);
    }

    if (pParser->isSet(m_clPasswordHex)) {
        const QByteArray baHex = pParser->value(m_clPasswordHex).toLatin1();
        bool bHexValid = !baHex.isEmpty() && ((baHex.size() & 1) == 0) && (baHex.size() <= 2 * 1024 * 1024);

        for (char ch : baHex) {
            if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))) {
                bHexValid = false;
                break;
            }
        }

        if (!bHexValid) {
            printf("Error: --password-hex requires an even, non-empty hexadecimal byte string\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }

        m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_PASSWORD_BYTES, QByteArray::fromHex(baHex));
    }

    if (pParser->isSet(m_clCodePage)) {
        bool bCodePageValid = false;
        const quint32 nCodePage = pParser->value(m_clCodePage).toUInt(&bCodePageValid);

        if (!bCodePageValid || (nCodePage == 0)) {
            printf("Error: --codepage requires a non-zero numeric Windows code page\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }

        m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_CODEPAGE, nCodePage);
    }

    return true;
}

bool XArchiveConsole::buildCommand(const QCommandLineParser *pParser, COMMAND *pCommand, XOptions::CR *pcrResult)
{
    pCommand->dialect = DIALECT_NATIVE;
    pCommand->bVerbose = pParser->isSet(m_clVerbose);
    pCommand->bQuiet = pParser->isSet(m_clQuiet);
    pCommand->bFlatten = pParser->isSet(m_clFlatten);
    pCommand->bIgnoreCase = pParser->isSet(m_clIgnoreCase);
    pCommand->sStruct = pParser->value(m_clStruct);
    pCommand->fileType = pParser->isSet(m_clFileType) ? XBinary::ftStringToFileTypeId(pParser->value(m_clFileType)) : XBinary::FT_UNKNOWN;

    // Operands: the first is the archive, the rest select members -- the POSIX
    // tar shape ("tar -x -f a.tar member ..."), which unzip shares.  -f adds
    // archives explicitly, so operands are then all member patterns.
    QStringList listOperands = pParser->positionalArguments();
    pCommand->listTargets = pParser->values(m_clFile);

    for (const QString &sOperand : listOperands) {
        // POSIX guideline 13 gives "-" the meaning of standard input.  Format
        // probing needs to seek, so say that plainly instead of failing later
        // with a confusing "cannot open".
        if (sOperand == QLatin1String("-")) {
            printf("Error: reading the archive from standard input is not supported; give a file name\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }

        if (pCommand->listTargets.isEmpty()) {
            pCommand->listTargets.append(sOperand);
        } else {
            pCommand->listIncludes.append(sOperand);
        }
    }

    pCommand->listIncludes += pParser->values(m_clInclude);
    pCommand->listExcludes = pParser->values(m_clExclude);

    if (pParser->isSet(m_clDirectory)) {
        pCommand->sOutputDirectory = pParser->value(m_clDirectory);
    }

    if (pParser->isSet(m_clExtractTo)) {
        pCommand->sOutputDirectory = pParser->value(m_clExtractTo);
    }

    // Overwrite policy: -k is the shorthand, --overwrite the explicit form.
    const QString sOverwrite = pParser->value(m_clOverwrite);

    if (pParser->isSet(m_clOverwrite)) {
        if (sOverwrite == QLatin1String("always")) pCommand->overwrite = OVERWRITE_ALWAYS;
        else if (sOverwrite == QLatin1String("skip")) pCommand->overwrite = OVERWRITE_SKIP;
        else if (sOverwrite == QLatin1String("rename")) pCommand->overwrite = OVERWRITE_RENAME;
        else {
            printf("Error: --overwrite requires always, skip, or rename\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }
    }

    if (pParser->isSet(m_clKeep)) {
        pCommand->overwrite = OVERWRITE_SKIP;
    }

    // One -o/--format for both the archive listing and the viewers, plus the
    // pre-POSIX one-switch-per-format spellings.
    QString sFormat = pParser->value(m_clFormat);

    if (!pParser->isSet(m_clFormat)) {
        if (pParser->isSet(m_clAsXml)) sFormat = QStringLiteral("xml");
        else if (pParser->isSet(m_clAsJson)) sFormat = QStringLiteral("json");
        else if (pParser->isSet(m_clAsCsv)) sFormat = QStringLiteral("csv");
        else if (pParser->isSet(m_clAsTsv)) sFormat = QStringLiteral("tsv");
        else if (pParser->isSet(m_clAsPlainText)) sFormat = QStringLiteral("text");
        else sFormat.clear();
    }

    if (!sFormat.isEmpty()) {
        if (sFormat == QLatin1String("native")) pCommand->listFormat = LISTFORMAT_NATIVE;
        else if (sFormat == QLatin1String("technical")) pCommand->listFormat = LISTFORMAT_TECHNICAL;
        else if (sFormat == QLatin1String("unzip")) pCommand->listFormat = LISTFORMAT_UNZIP;
        else if (sFormat == QLatin1String("unzip-verbose")) pCommand->listFormat = LISTFORMAT_UNZIP_VERBOSE;
        else if (sFormat == QLatin1String("zipinfo")) pCommand->listFormat = LISTFORMAT_ZIPINFO;
        else if (sFormat == QLatin1String("json")) {
            pCommand->listFormat = LISTFORMAT_JSON;
            pCommand->resultFormat = RESULTFORMAT_JSON;
        } else if (sFormat == QLatin1String("xml")) pCommand->resultFormat = RESULTFORMAT_XML;
        else if (sFormat == QLatin1String("csv")) pCommand->resultFormat = RESULTFORMAT_CSV;
        else if (sFormat == QLatin1String("tsv")) pCommand->resultFormat = RESULTFORMAT_TSV;
        else if (sFormat == QLatin1String("text")) pCommand->resultFormat = RESULTFORMAT_TEXT;
        else {
            printf("Error: -o/--format requires native, technical, unzip, unzip-verbose, zipinfo, json, xml, csv, tsv, or text\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }
    } else if (pCommand->bVerbose) {
        pCommand->listFormat = LISTFORMAT_TECHNICAL;
    }

    // Resolve the operation.  POSIX leaves the order of options free, so the
    // operation is decided by which letter is present, never by position; two
    // of them is a usage error rather than a silent precedence rule.
    struct OPERATION {
        const QCommandLineOption *pOption;
        VERB verb;
        const char *pszName;
    };

    const OPERATION operations[] = {
        {&m_clList, VERB_LIST, "-t/-l/--list"},
        {&m_clExtract, VERB_EXTRACT, "-x/--extract"},
        {&m_clExtractTo, VERB_EXTRACT, "--extractarchive"},
        {&m_clVerify, VERB_TEST, "-W/--verify"},
        {&m_clToStdout, VERB_STDOUT, "-O/--to-stdout"},
        {&m_clInfo, VERB_INFO, "-i/--info"},
        {&m_clEntropy, VERB_ENTROPY, "-e/--entropy"},
        {&m_clStruct, VERB_STRUCT, "-s/--struct"},
        {&m_clStructs, VERB_SHOWSTRUCTS, "-S/--structs"},
        {&m_clFormats, VERB_FORMATS, "--formats"},
    };

    const qint32 nNumberOfOperations = qint32(sizeof(operations) / sizeof(operations[0]));
    QString sSelectedName;

    for (qint32 i = 0; i < nNumberOfOperations; i++) {
        if (!pParser->isSet(*(operations[i].pOption))) {
            continue;
        }

        // -x and --extractarchive are the same operation spelled two ways, so
        // naming both is not a conflict.
        if ((pCommand->verb != VERB_NONE) && (pCommand->verb != operations[i].verb)) {
            printf("Error: %s and %s select different operations; use one\n", sSelectedName.toUtf8().data(), operations[i].pszName);
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }

        pCommand->verb = operations[i].verb;
        sSelectedName = QString(operations[i].pszName);
    }

    // No operation letter: identify the operand, which is the useful answer
    // for a bare target in a tool whose job is opening unknown containers.
    if ((pCommand->verb == VERB_NONE) && !pCommand->listTargets.isEmpty()) {
        pCommand->verb = VERB_INFO;
    }

    return true;
}

// ---------------------------------------------------------------------------
// Dialects
// ---------------------------------------------------------------------------

bool XArchiveConsole::isSevenZipVerb(const QString &sToken)
{
    return (sToken == QLatin1String("l")) || (sToken == QLatin1String("x")) || (sToken == QLatin1String("e")) || (sToken == QLatin1String("t")) ||
           (sToken == QLatin1String("i"));
}

XArchiveConsole::DIALECT XArchiveConsole::detectDialect(const QString &sProgramName, const QStringList &listArguments)
{
    const QString sName = QFileInfo(sProgramName).completeBaseName().toLower();

    if ((sName == QLatin1String("7z")) || (sName == QLatin1String("7za")) || (sName == QLatin1String("7zr"))) {
        return DIALECT_SEVENZIP;
    }

    if (sName == QLatin1String("unzip")) {
        return DIALECT_UNZIP;
    }

    if (sName == QLatin1String("zipinfo")) {
        return DIALECT_ZIPINFO;
    }

    if (!listArguments.isEmpty()) {
        const QString sFirst = listArguments.at(0);

        if (sFirst == QLatin1String("unzip")) {
            return DIALECT_UNZIP;
        }

        if (sFirst == QLatin1String("zipinfo")) {
            return DIALECT_ZIPINFO;
        }

        // A real file always wins over a verb.  "l" and "x" are legal file
        // names, and opening one must not turn into a command because of where
        // it happens to sit on the line.  "--" forces the file reading.
        if (isSevenZipVerb(sFirst) && !QFileInfo::exists(sFirst)) {
            return DIALECT_SEVENZIP;
        }
    }

    return DIALECT_NATIVE;
}

bool XArchiveConsole::processForeignDialect(const QStringList &listArguments, qint32 *pnResult)
{
    if (listArguments.isEmpty()) {
        return false;
    }

    const QString sProgramName = listArguments.at(0);
    QStringList listTokens = listArguments.mid(1);
    const DIALECT dialect = detectDialect(sProgramName, listTokens);

    if (dialect == DIALECT_NATIVE) {
        return false;
    }

    // Drop an explicit selector token ("xfileunpackerc unzip -l foo.zip").
    if (!listTokens.isEmpty() && ((listTokens.at(0) == QLatin1String("unzip")) || (listTokens.at(0) == QLatin1String("zipinfo")))) {
        listTokens.removeFirst();
    }

    COMMAND command;
    command.dialect = dialect;

    XOptions::CR crResult = XOptions::CR_SUCCESS;
    bool bParsed = false;

    if (dialect == DIALECT_SEVENZIP) {
        bParsed = parseSevenZip(listTokens, &command, &crResult);
    } else {
        bParsed = parseUnzip(listTokens, dialect, &command, &crResult);
    }

    if (!bParsed) {
        if (pnResult) *pnResult = crResult;
        return true;
    }

    if (command.verb == VERB_NONE) {
        printf("%s", getDialectHelp(dialect, sProgramName).toUtf8().data());
        if (pnResult) *pnResult = XOptions::CR_SUCCESS;
        return true;
    }

    const XOptions::CR crExecute = execute(command);

    if (pnResult) {
        if (crExecute != XOptions::CR_SUCCESS) {
            *pnResult = crExecute;
        } else if (m_bProbeTimeoutOccurred) {
            *pnResult = XOptions::CR_PROBETIMEOUT;
        } else {
            *pnResult = XOptions::CR_SUCCESS;
        }
    }

    return true;
}

// Options this project adds on top of the foreign grammars.  Neither 7-Zip nor
// Info-ZIP uses GNU-style long options -- 7-Zip spells its stop marker as a
// bare "--" -- so the whole "--word" namespace is free in every dialect.
bool XArchiveConsole::applyLongOption(const QString &sToken, COMMAND *pCommand, XOptions::CR *pcrResult)
{
    QString sName = sToken.mid(2);
    QString sValue;
    const qint32 nEqual = sName.indexOf(QChar('='));

    if (nEqual >= 0) {
        sValue = sName.mid(nEqual + 1);
        sName = sName.left(nEqual);
    }

    if (sName == QLatin1String("verbose")) {
        pCommand->bVerbose = true;
    } else if (sName == QLatin1String("quiet")) {
        pCommand->bQuiet = true;
    } else if (sName == QLatin1String("nocolor")) {
        XOptions::setNoColor(true);
    } else if (sName == QLatin1String("flatten")) {
        pCommand->bFlatten = true;
    } else if (sName == QLatin1String("ignore-case")) {
        pCommand->bIgnoreCase = true;
    } else if (sName == QLatin1String("stoponerror")) {
        m_mapUnpackProperties.remove(XBinary::UNPACK_PROP_CONTINUEONERROR);
    } else if (sName == QLatin1String("json")) {
        pCommand->listFormat = LISTFORMAT_JSON;
    } else if (sName == QLatin1String("include")) {
        if (!sValue.isEmpty()) pCommand->listIncludes.append(sValue);
    } else if (sName == QLatin1String("exclude")) {
        if (!sValue.isEmpty()) pCommand->listExcludes.append(sValue);
    } else if (sName == QLatin1String("outdir")) {
        pCommand->sOutputDirectory = sValue;
    } else if (sName == QLatin1String("password")) {
        m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_PASSWORD, sValue);
    } else if (sName == QLatin1String("codepage")) {
        bool bValid = false;
        const quint32 nCodePage = sValue.toUInt(&bValid);
        if (!bValid || (nCodePage == 0)) {
            printf("Error: --codepage requires a non-zero numeric Windows code page\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }
        m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_CODEPAGE, nCodePage);
    } else if (sName == QLatin1String("password-hex")) {
        const QByteArray baHex = sValue.toLatin1();
        bool bHexValid = !baHex.isEmpty() && ((baHex.size() & 1) == 0) && (baHex.size() <= 2 * 1024 * 1024);
        for (char ch : baHex) {
            if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))) {
                bHexValid = false;
                break;
            }
        }
        if (!bHexValid) {
            printf("Error: --password-hex requires an even, non-empty hexadecimal byte string\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }
        m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_PASSWORD_BYTES, QByteArray::fromHex(baHex));
    } else if (sName == QLatin1String("probe-timeout")) {
        bool bValid = false;
        const qint64 nTimeout = sValue.toLongLong(&bValid);
        if (!bValid || (nTimeout < 0)) {
            printf("Error: --probe-timeout requires a non-negative number of milliseconds\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }
        m_nProbeTimeout = nTimeout;
    } else if (sName == QLatin1String("filetype")) {
        pCommand->fileType = XBinary::ftStringToFileTypeId(sValue);
    } else if (sName == QLatin1String("overwrite")) {
        if (sValue == QLatin1String("always")) pCommand->overwrite = OVERWRITE_ALWAYS;
        else if (sValue == QLatin1String("skip")) pCommand->overwrite = OVERWRITE_SKIP;
        else if (sValue == QLatin1String("rename")) pCommand->overwrite = OVERWRITE_RENAME;
        else {
            printf("Error: --overwrite requires always, skip, or rename\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }
    } else if (sName == QLatin1String("format")) {
        if (sValue == QLatin1String("native")) pCommand->listFormat = LISTFORMAT_NATIVE;
        else if (sValue == QLatin1String("technical")) pCommand->listFormat = LISTFORMAT_TECHNICAL;
        else if (sValue == QLatin1String("unzip")) pCommand->listFormat = LISTFORMAT_UNZIP;
        else if (sValue == QLatin1String("unzip-verbose")) pCommand->listFormat = LISTFORMAT_UNZIP_VERBOSE;
        else if (sValue == QLatin1String("zipinfo")) pCommand->listFormat = LISTFORMAT_ZIPINFO;
        else if (sValue == QLatin1String("json")) pCommand->listFormat = LISTFORMAT_JSON;
        else {
            printf("Error: --format requires native, technical, unzip, unzip-verbose, zipinfo, or json\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }
    } else if (sName == QLatin1String("help")) {
        pCommand->verb = VERB_NONE;
    } else {
        printf("Error: unknown option --%s\n", sName.toUtf8().data());
        if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
        return false;
    }

    return true;
}

bool XArchiveConsole::parseSevenZip(const QStringList &listArguments, COMMAND *pCommand, XOptions::CR *pcrResult)
{
    bool bStopSwitches = false;
    bool bVerbSeen = false;
    bool bStdoutRequested = false;

    for (qint32 i = 0; i < listArguments.count(); i++) {
        const QString sToken = listArguments.at(i);

        if (!bStopSwitches && (sToken == QLatin1String("--"))) {
            bStopSwitches = true;
            continue;
        }

        if (!bStopSwitches && sToken.startsWith(QLatin1String("--"))) {
            if (!applyLongOption(sToken, pCommand, pcrResult)) {
                return false;
            }
            continue;
        }

        if (!bStopSwitches && sToken.startsWith(QChar('-')) && (sToken.length() > 1)) {
            const QString sSwitch = sToken.mid(1);

            if (sSwitch.startsWith(QLatin1String("o"))) {
                pCommand->sOutputDirectory = sSwitch.mid(1);
            } else if (sSwitch.startsWith(QLatin1String("p"))) {
                m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_PASSWORD, sSwitch.mid(1));
            } else if (sSwitch == QLatin1String("y")) {
                pCommand->overwrite = OVERWRITE_ALWAYS;
            } else if (sSwitch.startsWith(QLatin1String("ao"))) {
                const QString sMode = sSwitch.mid(2);
                if (sMode == QLatin1String("a")) pCommand->overwrite = OVERWRITE_ALWAYS;
                else if (sMode == QLatin1String("s")) pCommand->overwrite = OVERWRITE_SKIP;
                else if ((sMode == QLatin1String("u")) || (sMode == QLatin1String("t"))) pCommand->overwrite = OVERWRITE_RENAME;
                else {
                    printf("Error: -ao requires a, s, t, or u\n");
                    if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
                    return false;
                }
            } else if (sSwitch == QLatin1String("so")) {
                bStdoutRequested = true;
            } else if (sSwitch == QLatin1String("slt")) {
                pCommand->listFormat = LISTFORMAT_TECHNICAL;
            } else if (sSwitch.startsWith(QLatin1String("i!")) || sSwitch.startsWith(QLatin1String("ir!"))) {
                pCommand->listIncludes.append(sSwitch.mid(sSwitch.indexOf(QChar('!')) + 1));
            } else if (sSwitch.startsWith(QLatin1String("x!")) || sSwitch.startsWith(QLatin1String("xr!"))) {
                pCommand->listExcludes.append(sSwitch.mid(sSwitch.indexOf(QChar('!')) + 1));
            } else if (sSwitch == QLatin1String("ssc-")) {
                pCommand->bIgnoreCase = true;
            } else if (sSwitch == QLatin1String("ssc")) {
                pCommand->bIgnoreCase = false;
            } else if (sSwitch.startsWith(QLatin1String("t"))) {
                pCommand->fileType = XBinary::ftStringToFileTypeId(sSwitch.mid(1));
            } else if (sSwitch.startsWith(QLatin1String("bso0")) || sSwitch.startsWith(QLatin1String("bsp0")) || (sSwitch == QLatin1String("bd"))) {
                pCommand->bQuiet = true;
            } else if (sSwitch.startsWith(QLatin1String("b")) || sSwitch.startsWith(QLatin1String("r")) || sSwitch.startsWith(QLatin1String("m")) ||
                       sSwitch.startsWith(QLatin1String("sn")) || sSwitch.startsWith(QLatin1String("sp")) || sSwitch.startsWith(QLatin1String("stl")) ||
                       sSwitch.startsWith(QLatin1String("w")) || sSwitch.startsWith(QLatin1String("sa"))) {
                // Accepted and ignored: compression tuning, recursion into
                // directories, NTFS metadata and pager/work-dir switches have no
                // effect on a read-only unpacker.  Rejecting them would break
                // otherwise valid 7-Zip command lines for no benefit.
            } else {
                printf("Error: unsupported 7-Zip switch -%s\n", sSwitch.toUtf8().data());
                if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
                return false;
            }

            continue;
        }

        if (!bVerbSeen) {
            bVerbSeen = true;

            if (sToken == QLatin1String("l")) pCommand->verb = VERB_LIST;
            else if (sToken == QLatin1String("x")) pCommand->verb = VERB_EXTRACT;
            else if (sToken == QLatin1String("e")) {
                pCommand->verb = VERB_EXTRACT;
                pCommand->bFlatten = true;
            } else if (sToken == QLatin1String("t")) pCommand->verb = VERB_TEST;
            else if (sToken == QLatin1String("i")) pCommand->verb = VERB_FORMATS;
            else {
                printf("Error: unsupported 7-Zip command '%s'\n", sToken.toUtf8().data());
                if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
                return false;
            }

            continue;
        }

        if (pCommand->listTargets.isEmpty()) {
            pCommand->listTargets.append(sToken);
        } else {
            pCommand->listIncludes.append(sToken);
        }
    }

    if (bStdoutRequested && (pCommand->verb == VERB_EXTRACT)) {
        pCommand->verb = VERB_STDOUT;
    }

    return true;
}

bool XArchiveConsole::parseUnzip(const QStringList &listArguments, DIALECT dialect, COMMAND *pCommand, XOptions::CR *pcrResult)
{
    bool bStopSwitches = false;
    bool bExcludeMode = false;
    bool bVerbSelected = false;

    if (dialect == DIALECT_ZIPINFO) {
        pCommand->verb = VERB_LIST;
        pCommand->listFormat = LISTFORMAT_ZIPINFO;
        bVerbSelected = true;
    }

    for (qint32 i = 0; i < listArguments.count(); i++) {
        const QString sToken = listArguments.at(i);

        if (!bStopSwitches && (sToken == QLatin1String("--"))) {
            bStopSwitches = true;
            continue;
        }

        if (!bStopSwitches && sToken.startsWith(QLatin1String("--"))) {
            if (!applyLongOption(sToken, pCommand, pcrResult)) {
                return false;
            }
            continue;
        }

        if (!bStopSwitches && sToken.startsWith(QChar('-')) && (sToken.length() > 1)) {
            // Info-ZIP clusters its switches: "-qo" is "-q -o", and a switch
            // that takes a value consumes the rest of the cluster or, when the
            // cluster ends, the next argument.
            const QString sCluster = sToken.mid(1);

            for (qint32 c = 0; c < sCluster.length(); c++) {
                const QChar cSwitch = sCluster.at(c);
                const QString sRest = sCluster.mid(c + 1);

                if (cSwitch == QChar('d')) {
                    if (!sRest.isEmpty()) {
                        pCommand->sOutputDirectory = sRest;
                        c = sCluster.length();
                    } else if ((i + 1) < listArguments.count()) {
                        pCommand->sOutputDirectory = listArguments.at(++i);
                    } else {
                        printf("Error: -d requires a directory\n");
                        if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
                        return false;
                    }
                } else if (cSwitch == QChar('P')) {
                    if (!sRest.isEmpty()) {
                        m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_PASSWORD, sRest);
                        c = sCluster.length();
                    } else if ((i + 1) < listArguments.count()) {
                        m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_PASSWORD, listArguments.at(++i));
                    } else {
                        printf("Error: -P requires a password\n");
                        if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
                        return false;
                    }
                } else if (cSwitch == QChar('Z')) {
                    pCommand->verb = VERB_LIST;
                    pCommand->listFormat = LISTFORMAT_ZIPINFO;
                    bVerbSelected = true;
                } else if (cSwitch == QChar('l')) {
                    if (pCommand->listFormat != LISTFORMAT_ZIPINFO) {
                        pCommand->verb = VERB_LIST;
                        pCommand->listFormat = LISTFORMAT_UNZIP;
                        bVerbSelected = true;
                    }
                } else if (cSwitch == QChar('v')) {
                    pCommand->verb = VERB_LIST;
                    if (pCommand->listFormat != LISTFORMAT_ZIPINFO) pCommand->listFormat = LISTFORMAT_UNZIP_VERBOSE;
                    pCommand->bVerbose = true;
                    bVerbSelected = true;
                } else if (cSwitch == QChar('t')) {
                    if (pCommand->listFormat != LISTFORMAT_ZIPINFO) {
                        pCommand->verb = VERB_TEST;
                        bVerbSelected = true;
                    }
                } else if ((cSwitch == QChar('p')) || (cSwitch == QChar('c'))) {
                    pCommand->verb = VERB_STDOUT;
                    bVerbSelected = true;
                } else if (cSwitch == QChar('j')) {
                    pCommand->bFlatten = true;
                } else if (cSwitch == QChar('o')) {
                    pCommand->overwrite = OVERWRITE_ALWAYS;
                } else if (cSwitch == QChar('n')) {
                    pCommand->overwrite = OVERWRITE_SKIP;
                } else if (cSwitch == QChar('q')) {
                    pCommand->bQuiet = true;
                } else if (cSwitch == QChar('C')) {
                    pCommand->bIgnoreCase = true;
                } else if (cSwitch == QChar('x')) {
                    bExcludeMode = true;
                } else if ((cSwitch == QChar('h')) || (cSwitch == QChar('?'))) {
                    pCommand->verb = VERB_NONE;
                    return true;
                } else if ((cSwitch == QChar('a')) || (cSwitch == QChar('b')) || (cSwitch == QChar('L')) || (cSwitch == QChar('X')) || (cSwitch == QChar('K')) ||
                           (cSwitch == QChar('M')) || (cSwitch == QChar('U')) || (cSwitch == QChar('T')) || (cSwitch == QChar('D')) || (cSwitch == QChar('s')) ||
                           (cSwitch == QChar('S')) || (cSwitch == QChar('V')) || (cSwitch == QChar('N')) || (cSwitch == QChar('B')) || (cSwitch == QChar('1')) ||
                           (cSwitch == QChar('m')) || (cSwitch == QChar('z')) || (cSwitch == QChar('f')) || (cSwitch == QChar('u'))) {
                    // Accepted and ignored: text conversion, permission and
                    // pager switches, and the zipinfo layout variants, none of
                    // which change which bytes come out of the archive.
                } else {
                    printf("Error: unsupported unzip switch -%s\n", QString(cSwitch).toUtf8().data());
                    if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
                    return false;
                }
            }

            continue;
        }

        if (pCommand->listTargets.isEmpty() && !bExcludeMode) {
            pCommand->listTargets.append(sToken);
        } else if (bExcludeMode) {
            pCommand->listExcludes.append(sToken);
        } else {
            pCommand->listIncludes.append(sToken);
        }
    }

    // unzip's default action is extraction.
    if (!bVerbSelected && (pCommand->verb == VERB_NONE) && !pCommand->listTargets.isEmpty()) {
        pCommand->verb = VERB_EXTRACT;
    }

    return true;
}

QString XArchiveConsole::getDialectHelp(DIALECT dialect, const QString &sProgramName)
{
    const QString sName = QFileInfo(sProgramName).completeBaseName();
    QString sResult;

    if (dialect == DIALECT_SEVENZIP) {
        sResult += QString("Usage: %1 <command> [<switches>...] <archive> [<members>...]\n\n").arg(sName);
        sResult += "<Commands>\n";
        sResult += "  l : List contents of archive\n";
        sResult += "  x : eXtract files with full paths\n";
        sResult += "  e : Extract files without using directory names\n";
        sResult += "  t : Test integrity of archive\n";
        sResult += "  i : Show information about supported formats\n\n";
        sResult += "<Switches>\n";
        sResult += "  -o{Directory}  : set Output directory\n";
        sResult += "  -p{Password}   : set Password\n";
        sResult += "  -y             : assume Yes on all queries\n";
        sResult += "  -ao{a|s|u}     : set Overwrite mode (always|skip|rename)\n";
        sResult += "  -i!{wildcard}  : Include member names\n";
        sResult += "  -x!{wildcard}  : eXclude member names\n";
        sResult += "  -so            : write data to stdout\n";
        sResult += "  -slt           : show technical information for l (List)\n";
        sResult += "  -t{Type}       : set type of archive\n";
        sResult += "  --             : stop switches parsing\n";
    } else if (dialect == DIALECT_ZIPINFO) {
        sResult += QString("Usage: %1 [options] archive[.zip] [member...] [-x xmember...]\n\n").arg(sName);
        sResult += "  Lists archive contents in zipinfo format.\n";
    } else {
        sResult += QString("Usage: %1 [-opts] file[.zip] [list] [-x xlist] [-d exdir]\n\n").arg(sName);
        sResult += "  -l  list files (short format)     -t  test compressed archive data\n";
        sResult += "  -v  list verbosely                -Z  ZipInfo-style listing\n";
        sResult += "  -p  extract files to pipe         -c  extract files to stdout\n";
        sResult += "  -d  extract files into exdir      -x  exclude the files that follow\n";
        sResult += "  -j  junk paths                    -C  match names case-insensitively\n";
        sResult += "  -o  overwrite without prompting   -n  never overwrite existing files\n";
        sResult += "  -q  quiet                         -P  use the given password\n";
    }

    sResult += "\nThis build also accepts its own long options in this dialect, for example\n";
    sResult += "--format=json, --password-hex, --codepage, --probe-timeout and --filetype.\n";

    return sResult;
}

// ---------------------------------------------------------------------------
// Native command line
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

void XArchiveConsole::setUnpackProperties(const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties)
{
    m_mapUnpackProperties = mapProperties;
}

QMap<XBinary::UNPACK_PROP, QVariant> XArchiveConsole::getUnpackProperties() const
{
    return m_mapUnpackProperties;
}

void XArchiveConsole::setProbeTimeout(qint64 nMilliseconds)
{
    m_nProbeTimeout = nMilliseconds;
}

qint64 XArchiveConsole::getProbeTimeout() const
{
    return m_nProbeTimeout;
}

bool XArchiveConsole::isProbeTimeoutOccurred() const
{
    return m_bProbeTimeoutOccurred;
}

void XArchiveConsole::clearProbeTimeoutOccurred()
{
    m_bProbeTimeoutOccurred = false;
}

QMap<XBinary::UNPACK_PROP, QVariant> XArchiveConsole::buildUnpackProperties(const COMMAND &command) const
{
    QMap<XBinary::UNPACK_PROP, QVariant> mapResult = m_mapUnpackProperties;

    // OVERWRITE_ALWAYS is the historical default and is what decompressToFolder
    // assumes when the property is absent; leave it unset so the extraction
    // core keeps its own defaulting.  OVERWRITE_RENAME is what that core
    // already does when overwriting is off and filename fixing is on, which is
    // 7-Zip's -aou / "extract next to it" behaviour.
    if (command.overwrite == OVERWRITE_SKIP) {
        // Only the dedicated skip flag: clearing UNPACK_PROP_OVERWRITEFILES as
        // well is neither needed nor harmless -- the skip decision happens
        // before the overwrite decision, and the cleared flag drives a separate
        // replace path in the publish step.
        mapResult.insert(XBinary::UNPACK_PROP_SKIPEXISTINGFILES, true);
    } else if (command.overwrite == OVERWRITE_ALWAYS) {
        mapResult.insert(XBinary::UNPACK_PROP_OVERWRITEFILES, true);
    }

    return mapResult;
}

// ---------------------------------------------------------------------------
// Member selection
// ---------------------------------------------------------------------------

// Wildcard matching is written out rather than delegated to
// QRegularExpression::wildcardToRegularExpression(): Qt 5 and Qt 6 disagree on
// whether '*' may cross a '/' separator, and this project builds against both.
// Archive member selection follows the unzip / 7-Zip convention, where it does.
static bool _xacWildcardCharEquals(QChar cPattern, QChar cText, Qt::CaseSensitivity caseSensitivity)
{
    if (caseSensitivity == Qt::CaseSensitive) {
        return cPattern == cText;
    }

    return cPattern.toLower() == cText.toLower();
}

static bool _xacMatchWildcard(const QString &sPattern, const QString &sText, Qt::CaseSensitivity caseSensitivity)
{
    qint32 nPattern = 0;
    qint32 nText = 0;
    qint32 nStarPattern = -1;
    qint32 nStarText = 0;

    while (nText < sText.length()) {
        if ((nPattern < sPattern.length()) &&
            ((sPattern.at(nPattern) == QChar('?')) || _xacWildcardCharEquals(sPattern.at(nPattern), sText.at(nText), caseSensitivity))) {
            nPattern++;
            nText++;
        } else if ((nPattern < sPattern.length()) && (sPattern.at(nPattern) == QChar('*'))) {
            nStarPattern = nPattern;
            nStarText = nText;
            nPattern++;
        } else if (nStarPattern >= 0) {
            nPattern = nStarPattern + 1;
            nStarText++;
            nText = nStarText;
        } else {
            return false;
        }
    }

    while ((nPattern < sPattern.length()) && (sPattern.at(nPattern) == QChar('*'))) {
        nPattern++;
    }

    return nPattern == sPattern.length();
}

bool XArchiveConsole::isMemberSelected(const QString &sMemberName, const QStringList &listIncludes, const QStringList &listExcludes, bool bIgnoreCase)
{
    const Qt::CaseSensitivity caseSensitivity = bIgnoreCase ? Qt::CaseInsensitive : Qt::CaseSensitive;
    QString sNormalized = sMemberName;
    sNormalized.replace(QChar('\\'), QChar('/'));

    for (const QString &sPattern : listExcludes) {
        QString sNormalizedPattern = sPattern;
        sNormalizedPattern.replace(QChar('\\'), QChar('/'));

        if (_xacMatchWildcard(sNormalizedPattern, sNormalized, caseSensitivity)) {
            return false;
        }

        // A bare directory name covers everything under it, the way both
        // 7-Zip and unzip treat "sub" as "sub/*".
        if (sNormalized.startsWith(sNormalizedPattern + QChar('/'), caseSensitivity)) {
            return false;
        }
    }

    if (listIncludes.isEmpty()) {
        return true;
    }

    for (const QString &sPattern : listIncludes) {
        QString sNormalizedPattern = sPattern;
        sNormalizedPattern.replace(QChar('\\'), QChar('/'));

        if (_xacMatchWildcard(sNormalizedPattern, sNormalized, caseSensitivity)) {
            return true;
        }

        if (sNormalized.startsWith(sNormalizedPattern + QChar('/'), caseSensitivity)) {
            return true;
        }
    }

    return false;
}

QList<XBinary::ARCHIVERECORD> XArchiveConsole::filterRecords(const QList<XBinary::ARCHIVERECORD> &listRecords, const COMMAND &command)
{
    if (command.listIncludes.isEmpty() && command.listExcludes.isEmpty()) {
        return listRecords;
    }

    QList<XBinary::ARCHIVERECORD> listResult;

    for (qint32 i = 0; i < listRecords.count(); i++) {
        if (isMemberSelected(getRecordName(listRecords.at(i)), command.listIncludes, command.listExcludes, command.bIgnoreCase)) {
            listResult.append(listRecords.at(i));
        }
    }

    return listResult;
}

// ---------------------------------------------------------------------------
// Executors
// ---------------------------------------------------------------------------

XOptions::CR XArchiveConsole::execute(const COMMAND &command)
{
    if (command.verb == VERB_LIST) return listArchives(command);
    if (command.verb == VERB_EXTRACT) return extractArchives(command);
    if (command.verb == VERB_TEST) return testArchives(command);
    if (command.verb == VERB_STDOUT) return writeMembersToStdout(command);
    if (command.verb == VERB_FORMATS) return listSupportedFormats(command);
    if (command.verb == VERB_SHOWSTRUCTS) return showStructsOverview(command);

    // The remaining verbs are per-file viewers.
    if ((command.verb == VERB_INFO) || (command.verb == VERB_ENTROPY) || (command.verb == VERB_STRUCT)) {
        if (command.listTargets.isEmpty()) {
            printf("Error: this operation requires a target\n");
            return XOptions::CR_INVALIDPARAMETER;
        }

        XOptions::CR result = XOptions::CR_SUCCESS;
        const bool bShowFileName = (command.listTargets.count() > 1);

        for (const QString &sFileName : command.listTargets) {
            if (bShowFileName) {
                printf("%s:\n", QDir().toNativeSeparators(sFileName).toUtf8().data());
            }

            if (!QFileInfo::exists(sFileName)) {
                printf("Cannot find: %s\n", sFileName.toUtf8().data());
                result = XOptions::CR_CANNOTFINDFILE;
                continue;
            }

            XOptions::CR crViewer = XOptions::CR_SUCCESS;

            if (command.verb == VERB_ENTROPY) crViewer = showFileEntropy(sFileName, command);
            else if (command.verb == VERB_STRUCT) crViewer = showFileStruct(sFileName, command);
            else crViewer = showFileInfo(sFileName, command);

            if (crViewer != XOptions::CR_SUCCESS) result = crViewer;
        }

        return result;
    }

    return XOptions::CR_SUCCESS;
}

XBinary::FT XArchiveConsole::detectFileType(QIODevice *pDevice, XBinary::FT fileType, bool bValidateArchiveType, XBinary::PDSTRUCT *pPdStruct)
{
    XBinary::FT result = fileType;

    if (!XFormats::isStaticUnpacker(result)) {
        if (m_nProbeTimeout == 0) {
            XBinary::disablePdStructDeadline(pPdStruct);
        } else {
            XBinary::setPdStructDeadline(pPdStruct, m_nProbeTimeout);
        }

        // A preliminary scan commonly reports a generic PE type. Probe the executable
        // for a specific packer/installer before sending it to the archive backends.
        const XBinary::FT ftStatic = XFormats::getPrefFileType(pDevice, XBinary::FT_FLAG_EXECUTABLES | XBinary::FT_FLAG_STATICUNPACKERS, pPdStruct);

        if (XFormats::isStaticUnpacker(ftStatic)) {
            result = ftStatic;
        } else if (result == XBinary::FT_UNKNOWN) {
            // Only re-probe for an archive type when the user did not force one
            // with -F.  Without this guard an explicit --filetype (e.g. UDF on a
            // bridge disc that, by definition, also carries an ISO 9660
            // descriptor) is silently overwritten by auto-detection.
            if (bValidateArchiveType) {
                const XBinary::FT ftArchive = XFormats::getPrefFileType(pDevice, XBinary::FT_FLAG_ARCHIVES | XBinary::FT_FLAG_STATICUNPACKERS, pPdStruct);

                if (XFormats::isStaticUnpacker(ftArchive) || XArchives::getArchiveOpenValidFileTypes().contains(ftArchive)) {
                    result = ftArchive;
                }
            } else {
                result = XFormats::getPrefFileType(pDevice, XBinary::FT_FLAG_ARCHIVES, pPdStruct);
            }
        }
    }

    return result;
}

XOptions::CR XArchiveConsole::listArchives(const QStringList &listFileNames, XBinary::FT fileType, bool bVerbose)
{
    COMMAND command;
    command.verb = VERB_LIST;
    command.listTargets = listFileNames;
    command.fileType = fileType;
    command.bVerbose = bVerbose;
    command.listFormat = bVerbose ? LISTFORMAT_TECHNICAL : LISTFORMAT_NATIVE;

    return listArchives(command);
}

XOptions::CR XArchiveConsole::listArchives(const COMMAND &command)
{
    XOptions::CR result = XOptions::CR_SUCCESS;

    if (command.listTargets.isEmpty()) {
        printf("Error: --listarchive requires <target>\n");
        return XOptions::CR_INVALIDPARAMETER;
    }

    const bool bShowFileName = (command.listTargets.count() > 1);

    for (const QString &sFileName : command.listTargets) {
        if (!QFileInfo::exists(sFileName)) {
            printf("Cannot find: %s\n", sFileName.toUtf8().data());
            result = XOptions::CR_CANNOTFINDFILE;
            continue;
        }

        if (bShowFileName && (command.listFormat != LISTFORMAT_UNZIP) && (command.listFormat != LISTFORMAT_UNZIP_VERBOSE) &&
            (command.listFormat != LISTFORMAT_ZIPINFO)) {
            printf("%s:\n", QDir().toNativeSeparators(sFileName).toUtf8().data());
        }

        QFile file;
        file.setFileName(sFileName);

        if (!file.open(QIODevice::ReadOnly)) {
            printf("Cannot open: %s\n", sFileName.toUtf8().data());
            result = XOptions::CR_CANNOTOPENFILE;
            continue;
        }

        XBinary::PDSTRUCT archivePdStruct = XBinary::createPdStruct();
        const XBinary::FT currentFileType = detectFileType(&file, command.fileType, true, &archivePdStruct);

        if (XBinary::isPdStructDeadlineExpired(&archivePdStruct)) {
            printf("Detection budget exceeded: %s\n", sFileName.toUtf8().data());
            m_bProbeTimeoutOccurred = true;
            result = XOptions::CR_PROBETIMEOUT;
            file.close();
            continue;
        }

        // The budget covers automatic format probing only. Enumeration
        // and extraction retain their existing cancellation semantics.
        XBinary::disablePdStructDeadline(&archivePdStruct);

        QList<XBinary::ARCHIVERECORD> listRecords;
        bool bListed = false;
        XBinary *pArchive = nullptr;

        if (XFormats::isStaticUnpacker(currentFileType) || XFormats::isArchive(currentFileType)) {
            pArchive = XFormats::createClass(currentFileType, &file);

            if (pArchive) {
                bool bComplete = false;
                listRecords = getRecords(pArchive, m_mapUnpackProperties, &archivePdStruct, &bComplete);
                bListed = bComplete;
            }
        }

        const qint64 nPhysicalSize = file.size();

        if (bListed) {
            const QList<XBinary::ARCHIVERECORD> listSelected = filterRecords(listRecords, command);
            QString sListing;

            if (command.listFormat == LISTFORMAT_UNZIP) {
                sListing = formatListUnzip(sFileName, listSelected, false);
            } else if (command.listFormat == LISTFORMAT_UNZIP_VERBOSE) {
                sListing = formatListUnzip(sFileName, listSelected, true);
            } else if (command.listFormat == LISTFORMAT_ZIPINFO) {
                sListing = formatListZipInfo(sFileName, listSelected, nPhysicalSize);
            } else if (command.listFormat == LISTFORMAT_JSON) {
                sListing = formatListJson(sFileName, currentFileType, listSelected, nPhysicalSize);
            } else {
                sListing = formatList(currentFileType, listSelected, nPhysicalSize, command.listFormat == LISTFORMAT_TECHNICAL);
            }

            printf("%s", sListing.toUtf8().data());
        } else {
            printf("Cannot open archive: %s\n", sFileName.toUtf8().data());

            if (command.bVerbose && (currentFileType != XBinary::FT_UNKNOWN)) {
                printf("  Detected: %s\n", XBinary::fileTypeIdToString(currentFileType).toUtf8().data());
            }

            const QString sArchiveError = XBinary::getPdStructErrorString(&archivePdStruct);

            if (command.bVerbose && !sArchiveError.isEmpty()) {
                printf("  %s\n", sArchiveError.toUtf8().data());
            }

            result = XOptions::CR_CANNOTOPENFILE;
        }

        delete pArchive;
        file.close();
    }

    return result;
}

XOptions::CR XArchiveConsole::extractArchives(const QString &sResultDirectory, const QStringList &listFileNames, XBinary::FT fileType, bool bVerbose)
{
    COMMAND command;
    command.verb = VERB_EXTRACT;
    command.sOutputDirectory = sResultDirectory;
    command.listTargets = listFileNames;
    command.fileType = fileType;
    command.bVerbose = bVerbose;

    return extractArchives(command);
}

XOptions::CR XArchiveConsole::extractArchives(const COMMAND &command)
{
    XOptions::CR result = XOptions::CR_SUCCESS;
    const QString sResultDirectory = command.sOutputDirectory.isEmpty() ? QDir::currentPath() : command.sOutputDirectory;

    if (command.listTargets.isEmpty()) {
        printf("Error: --extractarchive requires <directory> <target>\n");
        return XOptions::CR_INVALIDPARAMETER;
    }

    // The native --extractarchive spelling always names a directory; the
    // foreign dialects may omit it and mean "here".
    if ((command.dialect == DIALECT_NATIVE) && command.sOutputDirectory.isEmpty()) {
        printf("Error: --extractarchive requires <directory> <target>\n");
        return XOptions::CR_INVALIDPARAMETER;
    }

    if (!QDir().mkpath(sResultDirectory)) {
        printf("Cannot create directory: %s\n", sResultDirectory.toUtf8().data());
        return XOptions::CR_INVALIDPARAMETER;
    }

    if (command.overwrite == OVERWRITE_RENAME) {
        // Auto-rename needs UNPACK_PROP_OVERWRITEFILES cleared, and that path
        // currently aborts the whole extraction with a sharing violation when
        // it publishes the first member -- even into an empty directory. That
        // is a defect in the extraction core, not something this front end can
        // work around, so refuse rather than abort mid-archive.
        printf("Error: --overwrite=rename (7-Zip -aou) is not available:\n");
        printf("       the extraction core fails to publish files with overwriting disabled\n");
        return XOptions::CR_INVALIDPARAMETER;
    }

    const bool bMemberSelection = !command.listIncludes.isEmpty() || !command.listExcludes.isEmpty() || command.bFlatten;

    if (bMemberSelection) {
        // Member filtering and path flattening have to be applied inside
        // XBinary::decompressToFolder, which owns the folder transaction,
        // canonical-root confinement, reparse-point rejection and duplicate
        // name resolution.  Re-implementing a per-member loop out here would
        // silently drop every one of those guarantees, so refuse instead of
        // pretending the option worked.
        printf("Error: member selection and path flattening are not implemented for extraction yet\n");
        printf("       (they are honoured by --listarchive and --stdout)\n");
        return XOptions::CR_INVALIDPARAMETER;
    }

    const QMap<XBinary::UNPACK_PROP, QVariant> mapProperties = buildUnpackProperties(command);

    for (const QString &sFileName : command.listTargets) {
        if (!QFileInfo::exists(sFileName)) {
            printf("Cannot find: %s\n", sFileName.toUtf8().data());
            result = XOptions::CR_CANNOTFINDFILE;
            continue;
        }

        QFile file;
        file.setFileName(sFileName);

        if (!file.open(QIODevice::ReadOnly)) {
            printf("Cannot open: %s\n", sFileName.toUtf8().data());
            result = XOptions::CR_CANNOTOPENFILE;
            continue;
        }

        XBinary::PDSTRUCT archivePdStruct = XBinary::createPdStruct();
        const XBinary::FT currentFileType = detectFileType(&file, command.fileType, false, &archivePdStruct);

        if (XBinary::isPdStructDeadlineExpired(&archivePdStruct)) {
            printf("Detection budget exceeded: %s\n", sFileName.toUtf8().data());
            m_bProbeTimeoutOccurred = true;
            result = XOptions::CR_PROBETIMEOUT;
            file.close();
            continue;
        }

        XBinary::disablePdStructDeadline(&archivePdStruct);

        qint32 nNumberOfFiles = 0;
        qint64 nTotalSize = 0;
        bool bTotalSizeComplete = true;
        QList<XBinary::ARCHIVERECORD> listRecords;
        XBinary *pArchive = nullptr;

        if (XFormats::isStaticUnpacker(currentFileType) || XFormats::isArchive(currentFileType)) {
            pArchive = XFormats::createClass(currentFileType, &file);

            if (pArchive) {
                listRecords = getRecords(pArchive, mapProperties, &archivePdStruct);
            }
        }

        for (qint32 i = 0; i < listRecords.count(); i++) {
            if (!isRecordFolder(listRecords.at(i))) {
                nNumberOfFiles++;

                if (isRecordSizePresent(listRecords.at(i))) {
                    nTotalSize += getRecordSize(listRecords.at(i));
                } else {
                    bTotalSizeComplete = false;
                }

                if (command.bVerbose) {
                    printf("  %s\n", getRecordName(listRecords.at(i)).toUtf8().data());
                }
            }
        }

        delete pArchive;

        XBinary::setPdStructErrorString(&archivePdStruct, QString());
        qint32 nSkippedEntries = 0;
        const bool bExtracted = file.seek(0) && XArchives::decompressToFolder(&file, sResultDirectory, mapProperties, &archivePdStruct, &nSkippedEntries);
        file.close();

        if (bExtracted) {
            const QString sTotalSize = bTotalSizeComplete ? XBinary::bytesCountToString(nTotalSize, 1024) : QString("unknown");

            if (nSkippedEntries > 0) {
                const qint32 nExtractedFiles = qMax(0, nNumberOfFiles - nSkippedEntries);
                if (!command.bQuiet) {
                    printf("Extracted %d of %d file(s) -> %s\n", nExtractedFiles, nNumberOfFiles, QDir().toNativeSeparators(sResultDirectory).toUtf8().data());
                }
                result = XOptions::CR_PARTIALRESULT;
            } else if (!command.bQuiet) {
                printf("Extracted %d file(s), %s -> %s\n", nNumberOfFiles, sTotalSize.toUtf8().data(), QDir().toNativeSeparators(sResultDirectory).toUtf8().data());
            }

            // Best-effort extraction commits with a skip tally in the
            // progress error string; surface it so a partial result is
            // never mistaken for a complete one.
            const QString sSkippedWarning = XBinary::getPdStructErrorString(&archivePdStruct);

            if (!sSkippedWarning.isEmpty()) {
                printf("  Warning: %s\n", sSkippedWarning.toUtf8().data());
            }
        } else {
            printf("Cannot extract: %s\n", sFileName.toUtf8().data());
            const QString sExtractionError = XBinary::getPdStructErrorString(&archivePdStruct);

            if (command.bVerbose && !sExtractionError.isEmpty()) {
                printf("  %s\n", sExtractionError.toUtf8().data());
            }

            result = XOptions::CR_CANNOTOPENFILE;
        }
    }

    return result;
}

XOptions::CR XArchiveConsole::testArchives(const COMMAND &command)
{
    XOptions::CR result = XOptions::CR_SUCCESS;

    if (command.listTargets.isEmpty()) {
        printf("Error: --testarchive requires <target>\n");
        return XOptions::CR_INVALIDPARAMETER;
    }

    for (const QString &sFileName : command.listTargets) {
        if (!QFileInfo::exists(sFileName)) {
            printf("Cannot find: %s\n", sFileName.toUtf8().data());
            result = XOptions::CR_CANNOTFINDFILE;
            continue;
        }

        XBinary::PDSTRUCT archivePdStruct = XBinary::createPdStruct();
        XBinary::disablePdStructDeadline(&archivePdStruct);

        const bool bTested = XArchives::testArchive(sFileName, m_mapUnpackProperties, &archivePdStruct);
        const QString sError = XBinary::getPdStructErrorString(&archivePdStruct);

        if (bTested) {
            if (!command.bQuiet) {
                printf("No errors detected in compressed data of %s\n", QDir().toNativeSeparators(sFileName).toUtf8().data());
            }

            if (!sError.isEmpty()) {
                printf("  Warning: %s\n", sError.toUtf8().data());
            }
        } else {
            printf("Test failed: %s\n", QDir().toNativeSeparators(sFileName).toUtf8().data());

            if (!sError.isEmpty()) {
                printf("  %s\n", sError.toUtf8().data());
            }

            result = XOptions::CR_CANNOTOPENFILE;
        }
    }

    return result;
}

XOptions::CR XArchiveConsole::writeMembersToStdout(const COMMAND &command)
{
    XOptions::CR result = XOptions::CR_SUCCESS;

    if (command.listTargets.isEmpty()) {
        printf("Error: --stdout requires <target>\n");
        return XOptions::CR_INVALIDPARAMETER;
    }

    QFile output;

    if (!output.open(stdout, QIODevice::WriteOnly | QIODevice::Unbuffered, QFileDevice::DontCloseHandle)) {
        printf("Error: cannot write to standard output\n");
        return XOptions::CR_CANNOTOPENFILE;
    }

    for (const QString &sFileName : command.listTargets) {
        if (!QFileInfo::exists(sFileName)) {
            fprintf(stderr, "Cannot find: %s\n", sFileName.toUtf8().data());
            result = XOptions::CR_CANNOTFINDFILE;
            continue;
        }

        QFile file;
        file.setFileName(sFileName);

        if (!file.open(QIODevice::ReadOnly)) {
            fprintf(stderr, "Cannot open: %s\n", sFileName.toUtf8().data());
            result = XOptions::CR_CANNOTOPENFILE;
            continue;
        }

        XBinary::PDSTRUCT archivePdStruct = XBinary::createPdStruct();
        const XBinary::FT currentFileType = detectFileType(&file, command.fileType, true, &archivePdStruct);

        if (XBinary::isPdStructDeadlineExpired(&archivePdStruct)) {
            fprintf(stderr, "Detection budget exceeded: %s\n", sFileName.toUtf8().data());
            m_bProbeTimeoutOccurred = true;
            result = XOptions::CR_PROBETIMEOUT;
            file.close();
            continue;
        }

        XBinary::disablePdStructDeadline(&archivePdStruct);

        QList<XBinary::ARCHIVERECORD> listRecords;
        XBinary *pArchive = nullptr;

        if (XFormats::isStaticUnpacker(currentFileType) || XFormats::isArchive(currentFileType)) {
            pArchive = XFormats::createClass(currentFileType, &file);

            if (pArchive) {
                listRecords = getRecords(pArchive, m_mapUnpackProperties, &archivePdStruct);
            }
        }

        delete pArchive;
        pArchive = nullptr;

        if (listRecords.isEmpty()) {
            fprintf(stderr, "Cannot open archive: %s\n", sFileName.toUtf8().data());
            result = XOptions::CR_CANNOTOPENFILE;
            file.close();
            continue;
        }

        const QList<XBinary::ARCHIVERECORD> listSelected = filterRecords(listRecords, command);
        qint32 nWritten = 0;

        for (qint32 i = 0; i < listSelected.count(); i++) {
            if (isRecordFolder(listSelected.at(i))) {
                continue;
            }

            const QString sMemberName = getRecordName(listSelected.at(i));

            if (!file.seek(0)) {
                break;
            }

            XBinary::PDSTRUCT memberPdStruct = XBinary::createPdStruct();
            XBinary::disablePdStructDeadline(&memberPdStruct);

            const QByteArray baData = XArchives::decompress(&file, sMemberName, &memberPdStruct, m_mapUnpackProperties);
            const QString sMemberError = XBinary::getPdStructErrorString(&memberPdStruct);

            if (baData.isEmpty() && !sMemberError.isEmpty()) {
                fprintf(stderr, "Cannot extract member: %s\n", sMemberName.toUtf8().data());
                fprintf(stderr, "  %s\n", sMemberError.toUtf8().data());
                result = XOptions::CR_PARTIALRESULT;
                continue;
            }

            if (!baData.isEmpty() && (output.write(baData) != baData.size())) {
                fprintf(stderr, "Cannot write member to standard output: %s\n", sMemberName.toUtf8().data());
                result = XOptions::CR_CANNOTOPENFILE;
                break;
            }

            nWritten++;
        }

        if (nWritten == 0) {
            fprintf(stderr, "No matching member in: %s\n", sFileName.toUtf8().data());
            result = XOptions::CR_CANNOTFINDFILE;
        }

        file.close();
    }

    output.flush();

    return result;
}

XOptions::CR XArchiveConsole::listSupportedFormats(const COMMAND &command)
{
    QStringList listNames;
    const QSet<XBinary::FT> stFileTypes = XArchives::getArchiveOpenValidFileTypes();

    for (QSet<XBinary::FT>::const_iterator it = stFileTypes.constBegin(); it != stFileTypes.constEnd(); ++it) {
        const QString sName = XBinary::fileTypeIdToString(*it);

        if (!sName.isEmpty()) {
            listNames.append(sName);
        }
    }

    listNames.sort(Qt::CaseInsensitive);

    if (command.listFormat == LISTFORMAT_JSON) {
        QJsonObject jsonRoot;
        QJsonArray jsonFormats;

        for (const QString &sName : listNames) {
            jsonFormats.append(sName);
        }

        jsonRoot.insert("formats", jsonFormats);
        jsonRoot.insert("count", listNames.count());

        printf("%s\n", QJsonDocument(jsonRoot).toJson(QJsonDocument::Indented).constData());
    } else {
        printf("Supported archive formats (%d):\n", listNames.count());

        for (const QString &sName : listNames) {
            printf("  %s\n", sName.toUtf8().data());
        }
    }

    return XOptions::CR_SUCCESS;
}

// ---------------------------------------------------------------------------
// Info-ZIP renderings
//
// These reproduce the shape of "unzip -l", "unzip -v" and "zipinfo" so a
// script that already parses those keeps working.  Fields this project's
// record model does not carry (Info-ZIP's version-made-by, for instance) are
// printed as "-" rather than invented, because a plausible-looking wrong value
// is worse for a parser than an obviously absent one.
// ---------------------------------------------------------------------------

QString XArchiveConsole::formatListUnzip(const QString &sArchiveName, const QList<XBinary::ARCHIVERECORD> &listRecords, bool bVerbose)
{
    QString sResult;

    sResult += QString("Archive:  %1\n").arg(QDir().toNativeSeparators(sArchiveName));

    qint64 nTotalSize = 0;
    qint64 nTotalPacked = 0;
    qint32 nNumberOfEntries = 0;

    if (bVerbose) {
        sResult += " Length   Method    Size  Cmpr    Date    Time   CRC-32   Name\n";
        sResult += "--------  ------  ------- ---- ---------- ----- --------  ----\n";
    } else {
        sResult += "  Length      Date    Time    Name\n";
        sResult += "---------  ---------- -----   ----\n";
    }

    for (qint32 i = 0; i < listRecords.count(); i++) {
        const XBinary::ARCHIVERECORD &record = listRecords.at(i);
        const qint64 nSize = isRecordSizePresent(record) ? getRecordSize(record) : 0;
        const QString sModified = getRecordModified(record);
        const QString sDateTime = sModified.isEmpty() ? QString("                ") : sModified.left(16);

        nTotalSize += nSize;
        nNumberOfEntries++;

        if (bVerbose) {
            qint64 nPacked = 0;
            const bool bPackedKnown = (getRecordPacked(record, &nPacked) == PACKEDSTATE_VALUE);
            if (bPackedKnown) nTotalPacked += nPacked;

            QString sMethod = XBinary::getHandleMethods(record.mapProperties);
            if (sMethod.isEmpty()) sMethod = QString("-");
            if (sMethod.length() > 6) sMethod = sMethod.left(6);

            QString sRatio = QString("-");
            if (bPackedKnown && (nSize > 0)) {
                sRatio = QString("%1%").arg(((nSize - nPacked) * 100) / nSize);
            }

            QString sCRC = getRecordCRC(record).toLower();
            if (sCRC.isEmpty()) sCRC = QString("--------");

            sResult += QString("%1  %2  %3 %4 %5 %6  %7\n")
                           .arg(QString::number(nSize).rightJustified(8))
                           .arg(sMethod.leftJustified(6))
                           .arg(bPackedKnown ? QString::number(nPacked).rightJustified(7) : QString("      -"))
                           .arg(sRatio.rightJustified(4))
                           .arg(sDateTime)
                           .arg(sCRC.rightJustified(8))
                           .arg(getRecordName(record));
        } else {
            sResult += QString("%1  %2   %3\n").arg(QString::number(nSize).rightJustified(9)).arg(sDateTime).arg(getRecordName(record));
        }
    }

    if (bVerbose) {
        sResult += "--------          -------  ---                            -------\n";
        QString sTotalRatio = QString("-");
        if ((nTotalSize > 0) && (nTotalPacked > 0)) {
            sTotalRatio = QString("%1%").arg(((nTotalSize - nTotalPacked) * 100) / nTotalSize);
        }
        sResult += QString("%1          %2 %3                            %4 file%5\n")
                       .arg(QString::number(nTotalSize).rightJustified(8))
                       .arg(QString::number(nTotalPacked).rightJustified(7))
                       .arg(sTotalRatio.rightJustified(4))
                       .arg(nNumberOfEntries)
                       .arg(nNumberOfEntries == 1 ? QString() : QString("s"));
    } else {
        sResult += "---------                     -------\n";
        sResult += QString("%1                     %2 file%3\n")
                       .arg(QString::number(nTotalSize).rightJustified(9))
                       .arg(nNumberOfEntries)
                       .arg(nNumberOfEntries == 1 ? QString() : QString("s"));
    }

    return sResult;
}

QString XArchiveConsole::formatListZipInfo(const QString &sArchiveName, const QList<XBinary::ARCHIVERECORD> &listRecords, qint64 nPhysicalSize)
{
    QString sResult;

    sResult += QString("Archive:  %1\n").arg(QDir().toNativeSeparators(sArchiveName));
    sResult += QString("Zip file size: %1 bytes, number of entries: %2\n").arg(nPhysicalSize).arg(listRecords.count());

    qint64 nTotalSize = 0;
    qint64 nTotalPacked = 0;
    qint32 nNumberOfEntries = 0;

    for (qint32 i = 0; i < listRecords.count(); i++) {
        const XBinary::ARCHIVERECORD &record = listRecords.at(i);
        const bool bIsFolder = isRecordFolder(record);
        const qint64 nSize = isRecordSizePresent(record) ? getRecordSize(record) : 0;

        qint64 nPacked = 0;
        if (getRecordPacked(record, &nPacked) == PACKEDSTATE_VALUE) nTotalPacked += nPacked;

        nTotalSize += nSize;
        nNumberOfEntries++;

        QString sPermissions;
        if (record.mapProperties.contains(XBinary::FPART_PROP_FILEMODE)) {
            sPermissions = getModeString(record.mapProperties.value(XBinary::FPART_PROP_FILEMODE).toUInt(), bIsFolder).left(10);
        } else {
            sPermissions = bIsFolder ? QString("drwxr-xr-x") : QString("-rw-r--r--");
        }

        // Info-ZIP prints "<version> <host-os>" here.  Neither is part of this
        // project's record model for most containers, so print what the record
        // actually has and a dash for the rest.
        QString sHostOs = record.mapProperties.value(XBinary::FPART_PROP_HOSTOS).toString();
        if (sHostOs.isEmpty()) sHostOs = QString("unk");
        if (sHostOs.length() > 3) sHostOs = sHostOs.left(3).toLower();

        QString sMethod = XBinary::getHandleMethods(record.mapProperties);
        if (sMethod.isEmpty()) sMethod = QString("----");
        if (sMethod.length() > 4) sMethod = sMethod.left(4);

        QString sDateTime = getRecordModified(record);
        if (sDateTime.length() >= 16) {
            const QDateTime dateTime = QDateTime::fromString(sDateTime, "yyyy-MM-dd hh:mm:ss");
            sDateTime = dateTime.isValid() ? dateTime.toString("yy-MMM-dd HH:mm") : sDateTime.left(15);
        } else {
            sDateTime = QString("              ");
        }

        sResult += QString("%1  %2 %3 %4 %5 %6 %7 %8\n")
                       .arg(sPermissions.leftJustified(10))
                       .arg(QString("-.-"))
                       .arg(sHostOs.leftJustified(3))
                       .arg(QString::number(nSize).rightJustified(8))
                       .arg(record.mapProperties.value(XBinary::FPART_PROP_ENCRYPTED).toBool() ? QString("B+") : QString("b-"))
                       .arg(sMethod.leftJustified(4))
                       .arg(sDateTime)
                       .arg(getRecordName(record));
    }

    QString sRatio = QString("0.0%");
    if ((nTotalSize > 0) && (nTotalPacked > 0)) {
        sRatio = QString("%1%").arg(QString::number(double(nTotalSize - nTotalPacked) * 100.0 / double(nTotalSize), 'f', 1));
    }

    sResult += QString("%1 file%2, %3 bytes uncompressed, %4 bytes compressed:  %5\n")
                   .arg(nNumberOfEntries)
                   .arg(nNumberOfEntries == 1 ? QString() : QString("s"))
                   .arg(nTotalSize)
                   .arg(nTotalPacked)
                   .arg(sRatio);

    return sResult;
}

QString XArchiveConsole::formatListJson(const QString &sArchiveName, XBinary::FT fileType, const QList<XBinary::ARCHIVERECORD> &listRecords, qint64 nPhysicalSize)
{
    QJsonObject jsonRoot;

    jsonRoot.insert("archive", QDir().toNativeSeparators(sArchiveName));
    jsonRoot.insert("fileType", XBinary::fileTypeIdToString(fileType));
    jsonRoot.insert("physicalSize", nPhysicalSize);

    QJsonArray jsonRecords;
    qint64 nTotalSize = 0;
    qint64 nTotalPacked = 0;
    qint32 nNumberOfFiles = 0;
    qint32 nNumberOfFolders = 0;

    for (qint32 i = 0; i < listRecords.count(); i++) {
        const XBinary::ARCHIVERECORD &record = listRecords.at(i);
        QJsonObject jsonRecord;

        if (isRecordFolder(record)) {
            nNumberOfFolders++;
        } else {
            nNumberOfFiles++;
        }

        if (isRecordSizePresent(record)) {
            nTotalSize += getRecordSize(record);
        }

        qint64 nPacked = 0;
        if (getRecordPacked(record, &nPacked) == PACKEDSTATE_VALUE) {
            nTotalPacked += nPacked;
        }

        // Emit the full property map: this is the whole point of a
        // machine-readable listing, and neither 7-Zip nor Info-ZIP exposes it.
        QList<XBinary::FPART_PROP> listKeys = record.mapProperties.keys();
        std::sort(listKeys.begin(), listKeys.end());

        for (qint32 k = 0; k < listKeys.count(); k++) {
            const XBinary::FPART_PROP prop = listKeys.at(k);

            if (prop == XBinary::FPART_PROP_ARCHIVE_RECORD_TOKEN) {
                continue;  // session bookkeeping, not archive metadata
            }

            const QVariant varValue = record.mapProperties.value(prop);
            const QString sKey = getPropertyName(prop);

            // Numbers stay numbers only where the number IS the value: sizes,
            // offsets and ids.  Everything else -- method, CRC, timestamps,
            // file mode -- is emitted as the rendered text, because the raw
            // enum ordinal ("Method": 3) means nothing outside this build and
            // would silently change when the enum grows.
            const bool bNumericQuantity =
                (prop == XBinary::FPART_PROP_UNCOMPRESSEDSIZE) || (prop == XBinary::FPART_PROP_COMPRESSEDSIZE) || (prop == XBinary::FPART_PROP_STREAMOFFSET) ||
                (prop == XBinary::FPART_PROP_STREAMSIZE) || (prop == XBinary::FPART_PROP_STREAMUNPACKEDSIZE) || (prop == XBinary::FPART_PROP_SUBSTREAMOFFSET) ||
                (prop == XBinary::FPART_PROP_UID) || (prop == XBinary::FPART_PROP_GID) || (prop == XBinary::FPART_PROP_WINDOWSIZE) ||
                (prop == XBinary::FPART_PROP_SOLIDFOLDERINDEX) || (prop == XBinary::FPART_PROP_ARCHIVE_RECORD_INDEX);

            if (varValue.userType() == QMetaType::Bool) {
                jsonRecord.insert(sKey, varValue.toBool());
            } else if (bNumericQuantity) {
                jsonRecord.insert(sKey, varValue.toLongLong());
            } else {
                jsonRecord.insert(sKey, getPropertyValueString(record, prop));
            }
        }

        jsonRecords.append(jsonRecord);
    }

    jsonRoot.insert("records", jsonRecords);
    jsonRoot.insert("numberOfFiles", nNumberOfFiles);
    jsonRoot.insert("numberOfFolders", nNumberOfFolders);
    jsonRoot.insert("totalSize", nTotalSize);
    jsonRoot.insert("totalPacked", nTotalPacked);

    return QString::fromUtf8(QJsonDocument(jsonRoot).toJson(QJsonDocument::Indented));
}
bool XArchiveConsole::hasAuthoritativeExternalStreamingReader(XBinary::FT fileType)
{
    switch (fileType) {
        case XBinary::FT_ZPAQ:
        case XBinary::FT_BCM:
        case XBinary::FT_LPAQ8:
        case XBinary::FT_PEA:
        case XBinary::FT_FREEARC: return true;
        default: return false;
    }
}

// Collect full archive records through the streaming unpack API so every
// property the format parser filled (method, timestamps, CRC, ownership, ...)
// is available, not just the handful the legacy flat RECORD struct carries.
QList<XBinary::ARCHIVERECORD> XArchiveConsole::getRecords(XBinary *pArchive, const QMap<XBinary::UNPACK_PROP, QVariant> &mapUnpackProperties, XBinary::PDSTRUCT *pPdStruct,
                                                    bool *pbComplete)
{
    QList<XBinary::ARCHIVERECORD> listResult;

    if (pbComplete) {
        *pbComplete = false;
    }

    if (!pArchive) {
        return listResult;
    }

    XBinary::UNPACK_STATE state = {};
    QMap<XBinary::UNPACK_PROP, QVariant> mapProperties = mapUnpackProperties;
    bool bInitialized = pArchive->initUnpack(&state, mapProperties, pPdStruct);

    const bool bPasswordSupplied = !mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString().isEmpty() ||
                                   !mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD_BYTES).toByteArray().isEmpty();
    if (!bInitialized && XBinary::isPdStructNotCanceled(pPdStruct) && !bPasswordSupplied && !hasAuthoritativeExternalStreamingReader(pArchive->getFileType())) {
        state = XBinary::UNPACK_STATE();
        mapProperties.insert(XBinary::UNPACK_PROP_METADATAONLY, true);
        bInitialized = pArchive->initUnpack(&state, mapProperties, pPdStruct);
    }

    if (bInitialized) {
        const qint32 nNumberOfRecords = state.nNumberOfRecords;
        bool bEnumerationComplete = (state.nCurrentIndex == 0) && (nNumberOfRecords >= 0) && (state.nCurrentIndex <= nNumberOfRecords);

        // The streaming contract advances only *between* declared records.
        // A single-record archive therefore finishes with currentIndex == 0;
        // moveToNext() returning false at that point is not an enumeration
        // failure.  Keep an independent bounded loop and validate that the
        // archive cannot silently change its declared count or cursor.
        for (qint32 i = 0; bEnumerationComplete && (i < nNumberOfRecords) && XBinary::isPdStructNotCanceled(pPdStruct); ++i) {
            const qint32 nExpectedIndex = state.nCurrentIndex;
            const XBinary::ARCHIVERECORD record = pArchive->infoCurrent(&state, pPdStruct);
            if (!XBinary::isPdStructNotCanceled(pPdStruct) || record.mapProperties.isEmpty() || !XBinary::isArchiveRecordExtentValid(record) ||
                (state.nCurrentIndex < 0) || (state.nCurrentIndex >= nNumberOfRecords) || (state.nCurrentIndex != nExpectedIndex) ||
                (state.nNumberOfRecords != nNumberOfRecords)) {
                bEnumerationComplete = false;
                break;
            }
            listResult.append(record);

            if (i + 1 < nNumberOfRecords) {
                const qint32 nPreviousIndex = state.nCurrentIndex;
                const bool bMoved = pArchive->moveToNext(&state, pPdStruct);
                if (!bMoved || !XBinary::isPdStructNotCanceled(pPdStruct) || (state.nCurrentIndex != (nPreviousIndex + 1)) || (state.nCurrentIndex >= nNumberOfRecords) ||
                    (state.nNumberOfRecords != nNumberOfRecords)) {
                    bEnumerationComplete = false;
                    break;
                }
            }
        }

        const bool bFinished = pArchive->finishUnpack(&state, nullptr);
        bEnumerationComplete = bEnumerationComplete && bFinished && (listResult.size() == nNumberOfRecords) && XBinary::isPdStructNotCanceled(pPdStruct);
        if (pbComplete) {
            *pbComplete = bEnumerationComplete;
        }
        if (!bEnumerationComplete) {
            listResult.clear();
        }
    }

    return listResult;
}

QString XArchiveConsole::getRecordName(const XBinary::ARCHIVERECORD &record)
{
    return record.mapProperties.value(XBinary::FPART_PROP_ORIGINALNAME).toString();
}

bool XArchiveConsole::isRecordFolder(const XBinary::ARCHIVERECORD &record)
{
    QString sName = getRecordName(record);

    return record.mapProperties.value(XBinary::FPART_PROP_ISFOLDER).toBool() || sName.endsWith(QChar('/')) || sName.endsWith(QChar('\\'));
}

bool XArchiveConsole::isRecordSizePresent(const XBinary::ARCHIVERECORD &record)
{
    if (!record.mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE)) {
        return false;
    }

    bool bOk = false;
    const qint64 nSize = record.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bOk);
    return bOk && (nSize >= 0);
}

qint64 XArchiveConsole::getRecordSize(const XBinary::ARCHIVERECORD &record)
{
    return isRecordSizePresent(record) ? record.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong() : 0;
}

// A record's stream coordinates are not always its own.  Two cases:
//   * an archive-stream record (HANDLE_METHOD_ARCHIVE_STREAM) publishes no
//     extent at all, because the member has no addressable extent on the
//     compressed device - the container's single stream holds every member;
//   * a solid record whose stream decodes to more than the member itself
//     (FPART_PROP_STREAMUNPACKEDSIZE > its uncompressed size) shares one
//     compressed stream with every other member of the block.
// In both cases nStreamSize is the container's number, not the member's, and
// printing it per row multiplies the archive size by the record count.
bool XArchiveConsole::isRecordSharingContainerStream(const XBinary::ARCHIVERECORD &record)
{
    qint32 nArchiveStreamIndex = -1;

    if (XBinary::getArchiveStreamRecordIndex(record, &nArchiveStreamIndex)) {
        return true;
    }

    if (record.mapProperties.value(XBinary::FPART_PROP_ISSOLID).toBool() && record.mapProperties.contains(XBinary::FPART_PROP_STREAMUNPACKEDSIZE)) {
        return record.mapProperties.value(XBinary::FPART_PROP_STREAMUNPACKEDSIZE).toLongLong() >
               record.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();
    }

    return false;
}

// Report the member's own packed size, never a number that belongs to the
// container.  An honest "unknown" is preferable to a confident wrong total.
XArchiveConsole::PACKEDSTATE XArchiveConsole::getRecordPacked(const XBinary::ARCHIVERECORD &record, qint64 *pnPacked)
{
    if (pnPacked) *pnPacked = 0;

    // Directories have no payload, so they have no packed size - not zero, not
    // the container's size.
    if (isRecordFolder(record)) {
        return PACKEDSTATE_NONE;
    }

    if (record.mapProperties.contains(XBinary::FPART_PROP_COMPRESSEDSIZE)) {
        if (pnPacked) *pnPacked = record.mapProperties.value(XBinary::FPART_PROP_COMPRESSEDSIZE).toLongLong();
        return PACKEDSTATE_VALUE;
    }

    if (isRecordSharingContainerStream(record)) {
        return PACKEDSTATE_UNKNOWN;
    }

    // Some static unpackers (including NSIS) publish one packed stream extent
    // on every solid member without a separate block id. Count that extent
    // before synthesizing zero for an empty member, otherwise a leading empty
    // member can claim the stream's deduplication key with a false zero.
    if (record.mapProperties.value(XBinary::FPART_PROP_ISSOLID).toBool() && (record.nStreamSize > 0)) {
        if (pnPacked) *pnPacked = record.nStreamSize;
        return PACKEDSTATE_VALUE;
    }

    // Only an explicitly reported zero denotes an empty regular file. A
    // missing size is unknown, not an implicit zero. Prefer an explicit packed
    // size above because an empty payload can still have nonzero framing.
    if (isRecordSizePresent(record) && (getRecordSize(record) == 0)) {
        return PACKEDSTATE_VALUE;
    }

    if (record.nStreamSize > 0) {
        if (pnPacked) *pnPacked = record.nStreamSize;
        return PACKEDSTATE_VALUE;
    }

    return PACKEDSTATE_UNKNOWN;
}

QString XArchiveConsole::getRecordPackedString(const XBinary::ARCHIVERECORD &record)
{
    qint64 nPacked = 0;

    if (getRecordPacked(record, &nPacked) == PACKEDSTATE_VALUE) {
        return QString::number(nPacked);
    }

    return QString();
}

QString XArchiveConsole::getRecordModified(const XBinary::ARCHIVERECORD &record)
{
    QDateTime dateTime;

    if (record.mapProperties.contains(XBinary::FPART_PROP_DATETIME)) {
        dateTime = record.mapProperties.value(XBinary::FPART_PROP_DATETIME).toDateTime();
    } else if (record.mapProperties.contains(XBinary::FPART_PROP_MTIME)) {
        dateTime = record.mapProperties.value(XBinary::FPART_PROP_MTIME).toDateTime();
    }

    if (dateTime.isValid()) {
        // absolute timestamps (7z FILETIME is UTC) are shown in the viewer's local
        // zone; local/wall-clock timestamps (DOS/ZIP) are already local, so
        // toLocalTime() leaves them unchanged
        return dateTime.toLocalTime().toString("yyyy-MM-dd hh:mm:ss");
    }

    return QString();
}

// Render a POSIX file mode as the familiar symbolic form plus octal, e.g.
// "-rwxr-xr-x (0755)".  Decimal mode values are unreadable on their own.
QString XArchiveConsole::getModeString(quint32 nMode, bool bIsFolder)
{
    QString sResult;

    quint32 nType = nMode & 0xF000u;  // S_IFMT

    if (nType == 0x4000u) sResult += QChar('d');       // S_IFDIR
    else if (nType == 0xA000u) sResult += QChar('l');  // S_IFLNK
    else if (nType == 0x8000u) sResult += QChar('-');  // S_IFREG
    else sResult += bIsFolder ? QChar('d') : QChar('-');

    const char cPerms[9] = {'r', 'w', 'x', 'r', 'w', 'x', 'r', 'w', 'x'};

    for (qint32 i = 0; i < 9; i++) {
        sResult += (nMode & (1u << (8 - i))) ? QChar(cPerms[i]) : QChar('-');
    }

    return QString("%1 (0%2)").arg(sResult).arg(nMode & 0777u, 0, 8);
}

QString XArchiveConsole::getRecordCRC(const XBinary::ARCHIVERECORD &record)
{
    XBinary::FPART_PROP prop = XBinary::FPART_PROP_UNKNOWN;

    if (record.mapProperties.contains(XBinary::FPART_PROP_RESULTCRC)) {
        prop = XBinary::FPART_PROP_RESULTCRC;
    } else if (record.mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDCRC)) {
        prop = XBinary::FPART_PROP_UNCOMPRESSEDCRC;
    }

    if (prop != XBinary::FPART_PROP_UNKNOWN) {
        return QString("%1").arg(record.mapProperties.value(prop).toULongLong(), 8, 16, QChar('0')).toUpper();
    }

    const QString sChecksum = record.mapProperties.value(XBinary::FPART_PROP_CHECKSUM).toString().trimmed();
    if (!sChecksum.isEmpty()) {
        return sChecksum.toUpper();
    }

    return QString();
}

QString XArchiveConsole::getRecordAttr(const XBinary::ARCHIVERECORD &record)
{
    QString sResult;

    // Match the technical-list convention used by UnRAR: R H A D S C I.
    // Compression/indexing are not currently exposed by the record model, so
    // those two positions remain dots.  Encryption is not a DOS attribute;
    // retain the app's existing '+' marker as an optional suffix.
    sResult += record.mapProperties.value(XBinary::FPART_PROP_ISREADONLY).toBool() ? QChar('R') : QChar('.');
    sResult += record.mapProperties.value(XBinary::FPART_PROP_ISHIDDEN).toBool() ? QChar('H') : QChar('.');
    sResult += record.mapProperties.value(XBinary::FPART_PROP_ISARCHIVE).toBool() ? QChar('A') : QChar('.');
    sResult += isRecordFolder(record) ? QChar('D') : QChar('.');
    sResult += record.mapProperties.value(XBinary::FPART_PROP_ISSYSTEM).toBool() ? QChar('S') : QChar('.');
    sResult += QStringLiteral("..");
    if (record.mapProperties.value(XBinary::FPART_PROP_ENCRYPTED).toBool()) {
        sResult += QChar('+');
    }

    return sResult;
}

QString XArchiveConsole::getRecordRatio(const XBinary::ARCHIVERECORD &record)
{
    if (isRecordFolder(record) || record.mapProperties.value(XBinary::FPART_PROP_ISSOLID).toBool() || !isRecordSizePresent(record)) {
        return QString();
    }

    const qint64 nSize = getRecordSize(record);
    qint64 nPacked = 0;
    if ((nSize <= 0) || (getRecordPacked(record, &nPacked) != PACKEDSTATE_VALUE)) {
        return QString();
    }

    return QString("%1%").arg(QString::number((double)nPacked * 100.0 / (double)nSize, 'f', 1));
}

QString XArchiveConsole::getPropertyName(XBinary::FPART_PROP prop)
{
    QString sResult;

    if (prop == XBinary::FPART_PROP_ORIGINALNAME) sResult = "Name";
    else if (prop == XBinary::FPART_PROP_UNCOMPRESSEDSIZE) sResult = "Size";
    else if (prop == XBinary::FPART_PROP_COMPRESSEDSIZE) sResult = "Compressed size";
    else if (prop == XBinary::FPART_PROP_HANDLEMETHOD) sResult = "Method";
    else if (prop == XBinary::FPART_PROP_HANDLEMETHOD2) sResult = "Method 2";
    else if (prop == XBinary::FPART_PROP_DATETIME) sResult = "Modified";
    else if (prop == XBinary::FPART_PROP_MTIME) sResult = "Modified";
    else if (prop == XBinary::FPART_PROP_CTIME) sResult = "Created";
    else if (prop == XBinary::FPART_PROP_ATIME) sResult = "Accessed";
    else if (prop == XBinary::FPART_PROP_RESULTCRC) sResult = "CRC";
    else if (prop == XBinary::FPART_PROP_UNCOMPRESSEDCRC) sResult = "CRC";
    else if (prop == XBinary::FPART_PROP_CRC_TYPE) sResult = "CRC type";
    else if (prop == XBinary::FPART_PROP_ENCRYPTED) sResult = "Encrypted";
    else if (prop == XBinary::FPART_PROP_FILEMODE) sResult = "Mode";
    else if (prop == XBinary::FPART_PROP_USERNAME) sResult = "User";
    else if (prop == XBinary::FPART_PROP_GROUPNAME) sResult = "Group";
    else if (prop == XBinary::FPART_PROP_UID) sResult = "UID";
    else if (prop == XBinary::FPART_PROP_GID) sResult = "GID";
    else if (prop == XBinary::FPART_PROP_LINKNAME) sResult = "Link";
    else if (prop == XBinary::FPART_PROP_INFO) sResult = "Info";
    else if (prop == XBinary::FPART_PROP_ISFOLDER) sResult = "Folder";
    else if (prop == XBinary::FPART_PROP_ISSOLID) sResult = "Solid";
    else if (prop == XBinary::FPART_PROP_SOLIDFOLDERINDEX) sResult = "Solid block";
    else if (prop == XBinary::FPART_PROP_WINDOWSIZE) sResult = "Window size";
    else if (prop == XBinary::FPART_PROP_STREAMOFFSET) sResult = "Stream offset";
    else if (prop == XBinary::FPART_PROP_STREAMSIZE) sResult = "Stream size";
    else if (prop == XBinary::FPART_PROP_STREAMUNPACKEDSIZE) sResult = "Stream unpacked size";
    else if (prop == XBinary::FPART_PROP_SUBSTREAMOFFSET) sResult = "Substream offset";
    else if (prop == XBinary::FPART_PROP_FILEMD5) sResult = "File MD5";
    else if (prop == XBinary::FPART_PROP_FLAGS) sResult = "Flags";
    else if (prop == XBinary::FPART_PROP_TYPE) sResult = "Raw method";
    else if (prop == XBinary::FPART_PROP_COMPRESSPROPERTIES) sResult = "Compress properties";
    else if (prop == XBinary::FPART_PROP_ISREADONLY) sResult = "Read-only";
    else if (prop == XBinary::FPART_PROP_ISHIDDEN) sResult = "Hidden";
    else if (prop == XBinary::FPART_PROP_ISSYSTEM) sResult = "System";
    else if (prop == XBinary::FPART_PROP_ISARCHIVE) sResult = "Archive attribute";
    else if (prop == XBinary::FPART_PROP_ISCOMMENTPRESENT) sResult = "Has comment";
    else if (prop == XBinary::FPART_PROP_ARCHIVE_RECORD_INDEX) sResult = "Archive record index";
    else if (prop == XBinary::FPART_PROP_ARCHIVE_RECORD_TOKEN) sResult = "Archive record token";
    else if (prop == XBinary::FPART_PROP_REPORTEDMETHOD) sResult = "Reported method";
    else if (prop == XBinary::FPART_PROP_HOSTOS) sResult = "Host OS";
    else if (prop == XBinary::FPART_PROP_CHECKSUM) sResult = "Checksum";
    else if (prop == XBinary::FPART_PROP_CHECKSUMTYPE) sResult = "Checksum type";
    else sResult = QString("#%1").arg(static_cast<qint32>(prop));

    return sResult;
}

QString XArchiveConsole::getPropertyValueString(const XBinary::ARCHIVERECORD &record, XBinary::FPART_PROP prop)
{
    QVariant varValue = record.mapProperties.value(prop);

    if (prop == XBinary::FPART_PROP_HANDLEMETHOD) {
        const QString sMethod = XBinary::getHandleMethods(record.mapProperties);

        if (!sMethod.isEmpty()) {
            return sMethod;
        }
    } else if ((prop == XBinary::FPART_PROP_HANDLEMETHOD2) || (prop == XBinary::FPART_PROP_HANDLEMETHOD3)) {
        QMap<XBinary::FPART_PROP, QVariant> mapOne;
        mapOne.insert(XBinary::FPART_PROP_HANDLEMETHOD, varValue);
        QString sMethod = XBinary::getHandleMethods(mapOne);

        if (!sMethod.isEmpty()) {
            return sMethod;
        }
    }

    if ((prop == XBinary::FPART_PROP_RESULTCRC) || (prop == XBinary::FPART_PROP_UNCOMPRESSEDCRC)) {
        return QString("%1").arg(varValue.toULongLong(), 8, 16, QChar('0')).toUpper();
    }

    if ((prop == XBinary::FPART_PROP_DATETIME) || (prop == XBinary::FPART_PROP_MTIME) || (prop == XBinary::FPART_PROP_CTIME) || (prop == XBinary::FPART_PROP_ATIME)) {
        return varValue.toDateTime().toLocalTime().toString("yyyy-MM-dd hh:mm:ss");
    }

    if (prop == XBinary::FPART_PROP_FILEMODE) {
        return getModeString(varValue.toUInt(), isRecordFolder(record));
    }

    if ((prop == XBinary::FPART_PROP_COMPRESSPROPERTIES) || (prop == XBinary::FPART_PROP_COMPRESSPROPERTIES2)) {
        QByteArray baProps = varValue.toByteArray();
        QString sHex = QString(baProps.toHex());
        XBinary::FPART_PROP methodProp = (prop == XBinary::FPART_PROP_COMPRESSPROPERTIES) ? XBinary::FPART_PROP_HANDLEMETHOD : XBinary::FPART_PROP_HANDLEMETHOD2;
        XBinary::HANDLE_METHOD handleMethod = (XBinary::HANDLE_METHOD)record.mapProperties.value(methodProp).toUInt();
        QString sDecoded = XBinary::getCoderParamsString(handleMethod, baProps);

        if (!sDecoded.isEmpty()) {
            return QString("%1 (%2:%3)").arg(sHex, XBinary::handleMethodToString(handleMethod), sDecoded);
        }

        return sHex;
    }

    if (varValue.userType() == QMetaType::Bool) {
        return varValue.toBool() ? QString("Yes") : QString("No");
    }

    if (varValue.userType() == QMetaType::QByteArray) {
        return QString(varValue.toByteArray().toHex());
    }

    return varValue.toString();
}

QString XArchiveConsole::getCellValue(const XBinary::ARCHIVERECORD &record, qint32 nColId)
{
    if (nColId == 0) return getRecordAttr(record);
    if (nColId == 1) return getRecordModified(record);
    if (nColId == 2) return isRecordSizePresent(record) ? QString::number(getRecordSize(record)) : QString();
    if (nColId == 3) return getRecordPackedString(record);
    if (nColId == 4) return XBinary::getHandleMethods(record.mapProperties);
    if (nColId == 5) return getRecordCRC(record);
    if (nColId == 7) return getRecordRatio(record);

    return getRecordName(record);
}

// Human-readable, aligned archive listing with a format/size summary line and
// (when bVerbose) a full per-record property dump.  Only columns the format
// actually populates are shown, so each archive type surfaces its own metadata.
QString XArchiveConsole::formatList(XBinary::FT fileType, const QList<XBinary::ARCHIVERECORD> &listRecords, qint64 nPhysicalSize, bool bVerbose)
{
    QString sResult;

    qint32 nNumberOfRecords = listRecords.count();

    qint64 nTotalSize = 0;
    qint64 nTotalPacked = 0;
    qint32 nNumberOfFiles = 0;
    qint32 nNumberOfFolders = 0;
    bool bAnyModified = false;
    bool bAnyMethod = false;
    bool bAnyCRC = false;
    bool bAnyAttr = false;
    bool bAnyRatio = false;
    bool bSolid = false;
    bool bSizeComplete = true;
    bool bPackedComplete = true;
    QSet<qint64> stBlocks;

    // Solid formats report the same shared compressed stream on every member of
    // a block, so count each distinct stream region once for the total and blank
    // the repeated "Packed" cells (the compressed size belongs on the first
    // member of a folder only).  Only records that actually declare solid-block
    // membership are folded this way: readers that report offset 0 for every
    // member would otherwise collapse distinct members
    // of equal size into one and understate the total.
    QStringList listPackedDisplay;
    QSet<QString> stSeenStreams;
    QSet<qint64> stSeenPackedBlocks;
    QSet<qint64> stBlocksWithPackedValue;
    QSet<qint64> stBlocksWithUnknownMembers;
    bool bUnknownPackedWithoutBlock = false;

    for (qint32 i = 0; i < nNumberOfRecords; i++) {
        const XBinary::ARCHIVERECORD &record = listRecords.at(i);

        if (isRecordSizePresent(record)) {
            nTotalSize += getRecordSize(record);
        } else if (!isRecordFolder(record)) {
            bSizeComplete = false;
        }

        if (isRecordFolder(record)) {
            nNumberOfFolders++;
        } else {
            nNumberOfFiles++;
        }

        if (record.mapProperties.value(XBinary::FPART_PROP_ISSOLID).toBool()) {
            bSolid = true;
        }

        if (record.mapProperties.contains(XBinary::FPART_PROP_SOLIDFOLDERINDEX)) {
            stBlocks.insert(record.mapProperties.value(XBinary::FPART_PROP_SOLIDFOLDERINDEX).toLongLong());
        }

        qint64 nPacked = 0;
        PACKEDSTATE packedState = getRecordPacked(record, &nPacked);
        QString sPackedDisplay;
        const bool bHasBlock = record.mapProperties.contains(XBinary::FPART_PROP_SOLIDFOLDERINDEX);
        const qint64 nBlock = bHasBlock ? record.mapProperties.value(XBinary::FPART_PROP_SOLIDFOLDERINDEX).toLongLong() : -1;

        if (packedState == PACKEDSTATE_UNKNOWN) {
            if (bHasBlock) {
                stBlocksWithUnknownMembers.insert(nBlock);
            } else {
                bUnknownPackedWithoutBlock = true;
            }
        } else if (packedState == PACKEDSTATE_VALUE) {
            sPackedDisplay = QString::number(nPacked);

            const bool bDeclaresSolidBlock = record.mapProperties.value(XBinary::FPART_PROP_ISSOLID).toBool() || bHasBlock;

            if (bHasBlock) {
                // getRecordPacked() synthesizes zero for an empty regular file.
                // That is a valid row value, but it is not evidence that the
                // provider supplied this solid folder's packed size.  Only an
                // actual packed-size/stream value can make unknown sibling
                // members complete.
                const bool bProviderPackedValue = record.mapProperties.contains(XBinary::FPART_PROP_COMPRESSEDSIZE) || (record.nStreamSize > 0);

                if (bProviderPackedValue) {
                    stBlocksWithPackedValue.insert(nBlock);

                    if (stSeenPackedBlocks.contains(nBlock)) {
                        sPackedDisplay.clear();
                    } else {
                        stSeenPackedBlocks.insert(nBlock);
                        nTotalPacked += nPacked;
                    }
                }
            } else if (bDeclaresSolidBlock && (record.nStreamSize > 0)) {
                QString sStreamKey = QString("%1:%2").arg(record.nStreamOffset).arg(record.nStreamSize);

                if (stSeenStreams.contains(sStreamKey)) {
                    sPackedDisplay.clear();  // already counted: this file shares the block
                } else {
                    stSeenStreams.insert(sStreamKey);
                    nTotalPacked += nPacked;
                }
            } else {
                nTotalPacked += nPacked;
            }
        }
        // PACKEDSTATE_NONE (directory): no cell, no contribution, and no reason
        // to call the total incomplete.

        listPackedDisplay.append(sPackedDisplay);

        if (!getRecordModified(record).isEmpty()) bAnyModified = true;
        if (!XBinary::getHandleMethods(record.mapProperties).isEmpty()) bAnyMethod = true;
        if (!getRecordCRC(record).isEmpty()) bAnyCRC = true;
        if (isRecordFolder(record) || record.mapProperties.value(XBinary::FPART_PROP_ISREADONLY).toBool() ||
            record.mapProperties.value(XBinary::FPART_PROP_ISHIDDEN).toBool() || record.mapProperties.value(XBinary::FPART_PROP_ISSYSTEM).toBool() ||
            record.mapProperties.value(XBinary::FPART_PROP_ISARCHIVE).toBool() || record.mapProperties.value(XBinary::FPART_PROP_ENCRYPTED).toBool())
            bAnyAttr = true;
        if (!getRecordRatio(record).isEmpty()) bAnyRatio = true;
    }

    // Missing per-member sizes are complete when every such member belongs to
    // a block for which the provider reported one folder-level packed size.
    // This is the kpidPackSize/kpidBlock contract used by solid 7z and RAR.
    if (bUnknownPackedWithoutBlock) {
        bPackedComplete = false;
    }
    for (qint64 nBlock : stBlocksWithUnknownMembers) {
        if (!stBlocksWithPackedValue.contains(nBlock)) {
            bPackedComplete = false;
            break;
        }
    }

    // A packed total can never exceed the bytes that are actually on disk.  If
    // the per-record numbers add up to more than the file itself, they are not
    // this container's packed sizes; report that they are unknown rather than
    // publishing a sum (and a ratio) that the file cannot support.
    if (bPackedComplete && (nPhysicalSize > 0) && (nTotalPacked > nPhysicalSize)) {
        bPackedComplete = false;
    }

    QString sRatio;

    if (bSizeComplete && bPackedComplete && (nTotalSize > 0)) {
        sRatio = QString(" (%1%)").arg(QString::number(double(nTotalPacked) * 100.0 / double(nTotalSize), 'f', 1));
    }

    const QString sSizeSummary = bSizeComplete ? XBinary::bytesCountToString(nTotalSize, 1024) : QString("unknown");
    const QString sPackedSummary = bPackedComplete ? XBinary::bytesCountToString(nTotalPacked, 1024) : QString("unknown");

    sResult += QString("%1: %2 file(s)%3, %4 -> %5%6\n")
                   .arg(XBinary::fileTypeIdToString(fileType))
                   .arg(nNumberOfFiles)
                   .arg(nNumberOfFolders > 0 ? QString(", %1 folder(s)").arg(nNumberOfFolders) : QString())
                   .arg(sSizeSummary)
                   .arg(sPackedSummary)
                   .arg(sRatio);

    // archive-level info line: on-disk size, metadata overhead, solidity
    QStringList listInfo;

    if (nPhysicalSize > 0) {
        listInfo.append(QString("physical %1").arg(XBinary::bytesCountToString(nPhysicalSize, 1024)));

        qint64 nOverhead = bPackedComplete ? (nPhysicalSize - nTotalPacked) : 0;

        if (nOverhead > 0) {
            listInfo.append(QString("overhead %1").arg(XBinary::bytesCountToString(nOverhead, 1024)));
        }
    }

    if (bSolid) {
        listInfo.append(QString("solid"));

        if (stBlocks.count() > 0) {
            listInfo.append(QString("%1 block(s)").arg(stBlocks.count()));
        }
    }

    if (!listInfo.isEmpty()) {
        sResult += QString("  %1\n").arg(listInfo.join(", "));
    }

    if (bVerbose) {
        for (qint32 i = 0; i < nNumberOfRecords; i++) {
            const XBinary::ARCHIVERECORD &record = listRecords.at(i);

            sResult += QString("\n[%1]\n").arg(getRecordName(record));

            QList<XBinary::FPART_PROP> listKeys = record.mapProperties.keys();
            std::sort(listKeys.begin(), listKeys.end());

            for (qint32 k = 0; k < listKeys.count(); k++) {
                XBinary::FPART_PROP prop = listKeys.at(k);

                if (prop == XBinary::FPART_PROP_ORIGINALNAME) {
                    continue;
                }

                // An opaque identity digest is bookkeeping between a record and
                // the session that produced it, not something the archive says
                // about this member.  A listing describes the member.
                if (prop == XBinary::FPART_PROP_ARCHIVE_RECORD_TOKEN) {
                    continue;
                }

                // The exact provider text is already rendered as Method via
                // getHandleMethods(); do not print a duplicate verbose field.
                if ((prop == XBinary::FPART_PROP_REPORTEDMETHOD) && record.mapProperties.contains(XBinary::FPART_PROP_HANDLEMETHOD)) {
                    continue;
                }

                // A stream coordinate describes where a payload lives.  A
                // directory entry has no payload, so no coordinate on any
                // device describes it and none may be shown for it.
                if (isRecordFolder(record) && ((prop == XBinary::FPART_PROP_STREAMOFFSET) || (prop == XBinary::FPART_PROP_STREAMSIZE) ||
                                               (prop == XBinary::FPART_PROP_STREAMUNPACKEDSIZE) || (prop == XBinary::FPART_PROP_SUBSTREAMOFFSET))) {
                    continue;
                }

                // A record that shares the container's single compressed
                // stream has no extent of its own either; printing the shared
                // one per row presents a transport-envelope number as this
                // member's own metadata, which is how "Stream size: <whole
                // container>" ended up on every row of a compressed-tar
                // listing.
                if (((prop == XBinary::FPART_PROP_STREAMOFFSET) || (prop == XBinary::FPART_PROP_STREAMSIZE)) && isRecordSharingContainerStream(record)) {
                    continue;
                }

                sResult += QString("  %1: %2\n").arg(getPropertyName(prop), getPropertyValueString(record, prop));
            }
        }

        return sResult;
    }

    // choose the columns present for this format (Name/Size/Packed always shown)
    QList<qint32> listColIds;
    QStringList listHeaders;
    QList<bool> listRightAlign;

    if (bAnyAttr) {
        listColIds.append(0);
        listHeaders.append("Attr");
        listRightAlign.append(false);
    }

    if (bAnyModified) {
        listColIds.append(1);
        listHeaders.append("Modified");
        listRightAlign.append(false);
    }

    listColIds.append(2);
    listHeaders.append("Size");
    listRightAlign.append(true);

    listColIds.append(3);
    listHeaders.append("Packed");
    listRightAlign.append(true);

    if (bAnyRatio) {
        listColIds.append(7);
        listHeaders.append("Ratio");
        listRightAlign.append(true);
    }

    if (bAnyMethod) {
        listColIds.append(4);
        listHeaders.append("Method");
        listRightAlign.append(false);
    }

    if (bAnyCRC) {
        listColIds.append(5);
        listHeaders.append("Checksum");
        listRightAlign.append(false);
    }

    listColIds.append(6);
    listHeaders.append("Name");
    listRightAlign.append(false);

    qint32 nNumberOfColumns = listColIds.count();

    QVector<qint32> vWidths(nNumberOfColumns);

    for (qint32 c = 0; c < nNumberOfColumns; c++) {
        vWidths[c] = listHeaders.at(c).length();
    }

    for (qint32 i = 0; i < nNumberOfRecords; i++) {
        for (qint32 c = 0; c < nNumberOfColumns; c++) {
            qint32 nColId = listColIds.at(c);
            QString sCell = (nColId == 3) ? listPackedDisplay.at(i) : getCellValue(listRecords.at(i), nColId);
            qint32 nLength = sCell.length();

            if (nLength > vWidths[c]) {
                vWidths[c] = nLength;
            }
        }
    }

    QString sHeader;
    QString sSeparator;

    for (qint32 c = 0; c < nNumberOfColumns; c++) {
        if (c > 0) {
            sHeader += "  ";
            sSeparator += "  ";
        }

        bool bLastColumn = (c == (nNumberOfColumns - 1));

        if (listRightAlign.at(c)) {
            sHeader += listHeaders.at(c).rightJustified(vWidths[c], QChar(' '));
        } else if (bLastColumn) {
            sHeader += listHeaders.at(c);
        } else {
            sHeader += listHeaders.at(c).leftJustified(vWidths[c], QChar(' '));
        }

        sSeparator += QString(vWidths[c], QChar('-'));
    }

    sResult += sHeader + "\n";
    sResult += sSeparator + "\n";

    for (qint32 i = 0; i < nNumberOfRecords; i++) {
        QString sRow;

        for (qint32 c = 0; c < nNumberOfColumns; c++) {
            if (c > 0) {
                sRow += "  ";
            }

            qint32 nColId = listColIds.at(c);
            QString sCell = (nColId == 3) ? listPackedDisplay.at(i) : getCellValue(listRecords.at(i), nColId);
            bool bLastColumn = (c == (nNumberOfColumns - 1));

            if (listRightAlign.at(c)) {
                sRow += sCell.rightJustified(vWidths[c], QChar(' '));
            } else if (bLastColumn) {
                sRow += sCell;
            } else {
                sRow += sCell.leftJustified(vWidths[c], QChar(' '));
            }
        }

        sResult += sRow + "\n";
    }

    return sResult;
}

// ---------------------------------------------------------------------------
// XFormats-backed viewers
//
// None of these needs a signature database or a scan engine, so an archive
// front end can carry them itself instead of borrowing a scan-engine console.
// ---------------------------------------------------------------------------

static QString _xacRenderItems(const QVector<XBinary::KeyValueItem> &listItems, XArchiveConsole::RESULTFORMAT resultFormat)
{
    if (resultFormat == XArchiveConsole::RESULTFORMAT_JSON) return XFormats::toJSON(listItems);
    if (resultFormat == XArchiveConsole::RESULTFORMAT_XML) return XFormats::toXML(listItems);
    if (resultFormat == XArchiveConsole::RESULTFORMAT_CSV) return XFormats::toCSV(listItems);
    if (resultFormat == XArchiveConsole::RESULTFORMAT_TSV) return XFormats::toTSV(listItems);

    return XFormats::toFormattedString(listItems);
}

XOptions::CR XArchiveConsole::showFileInfo(const QString &sFileName, const COMMAND &command)
{
    XOptions::CR result = XOptions::CR_SUCCESS;

    QFile file;
    file.setFileName(sFileName);

    if (file.open(QIODevice::ReadOnly)) {
        XBinary::PDSTRUCT pdStruct = XBinary::createPdStruct();
        const QVector<XBinary::KeyValueItem> listItems = XFormats::getFileInfo(&file, false, -1, &pdStruct, command.fileType);

        printf("%s", _xacRenderItems(listItems, command.resultFormat).toUtf8().data());
        file.close();
    } else {
        printf("Cannot open: %s\n", sFileName.toUtf8().data());
        result = XOptions::CR_CANNOTOPENFILE;
    }

    return result;
}

XOptions::CR XArchiveConsole::showFileEntropy(const QString &sFileName, const COMMAND &command)
{
    XOptions::CR result = XOptions::CR_SUCCESS;

    QFile file;
    file.setFileName(sFileName);

    if (file.open(QIODevice::ReadOnly)) {
        XBinary::PDSTRUCT pdStruct = XBinary::createPdStruct();
        const QVector<XBinary::KeyValueItem> listItems = XFormats::getEntropy(&file, false, -1, &pdStruct);

        printf("%s", _xacRenderItems(listItems, command.resultFormat).toUtf8().data());
        file.close();
    } else {
        printf("Cannot open: %s\n", sFileName.toUtf8().data());
        result = XOptions::CR_CANNOTOPENFILE;
    }

    return result;
}

XOptions::CR XArchiveConsole::showFileStruct(const QString &sFileName, const COMMAND &command)
{
    XOptions::CR result = XOptions::CR_SUCCESS;

    QFile file;
    file.setFileName(sFileName);

    if (file.open(QIODevice::ReadOnly)) {
        XBinary::PDSTRUCT pdStruct = XBinary::createPdStruct();
        const XBinary::XFHEADER xFHeader = XFormats::getXFHeaderFromStructName(&file, command.sStruct, false, -1, &pdStruct);

        if (xFHeader.xfType != XBinary::XFTYPE_UNKNOWN) {
            XBinary *pBinary = XFormats::createClass(xFHeader.fileType, &file);

            if (pBinary) {
                QString sStructInfo;
                XBinary::INDATA inData = XFormats::createINDATA(xFHeader.fileType, &file);

                if (xFHeader.xfType == XBinary::XFTYPE_HEADER) {
                    XFModel_header modelHeader(nullptr);
                    modelHeader.setData(inData, xFHeader);

                    if (command.resultFormat == RESULTFORMAT_JSON) sStructInfo = modelHeader.toJSON();
                    else if (command.resultFormat == RESULTFORMAT_XML) sStructInfo = modelHeader.toXML();
                    else if (command.resultFormat == RESULTFORMAT_CSV) sStructInfo = XFModel::exportToString(&modelHeader, XFModel::EXPORT_CSV);
                    else if (command.resultFormat == RESULTFORMAT_TSV) sStructInfo = XFModel::exportToString(&modelHeader, XFModel::EXPORT_TSV);
                    else XOptions::printModel(&modelHeader);
                } else if (xFHeader.xfType == XBinary::XFTYPE_TABLE) {
                    XFModel_table modelTable;
                    modelTable.setData(inData, xFHeader);
                    modelTable.setShowPresentation(true);

                    if (command.resultFormat == RESULTFORMAT_JSON) sStructInfo = modelTable.toJSON();
                    else if (command.resultFormat == RESULTFORMAT_XML) sStructInfo = modelTable.toXML();
                    else if (command.resultFormat == RESULTFORMAT_CSV) sStructInfo = XFModel::exportToString(&modelTable, XFModel::EXPORT_CSV);
                    else if (command.resultFormat == RESULTFORMAT_TSV) sStructInfo = XFModel::exportToString(&modelTable, XFModel::EXPORT_TSV);
                    else XOptions::printModel(&modelTable);
                }

                if (!sStructInfo.isEmpty()) {
                    printf("%s", sStructInfo.toUtf8().data());
                }

                delete pBinary;
            } else {
                printf("Cannot read structure: %s\n", sFileName.toUtf8().data());
                result = XOptions::CR_CANNOTOPENFILE;
            }
        } else {
            printf("Cannot find struct '%s': %s\n", command.sStruct.toUtf8().data(), sFileName.toUtf8().data());
            result = XOptions::CR_INVALIDPARAMETER;
        }

        file.close();
    } else {
        printf("Cannot open: %s\n", sFileName.toUtf8().data());
        result = XOptions::CR_CANNOTOPENFILE;
    }

    return result;
}

XOptions::CR XArchiveConsole::showStructsOverview(const COMMAND &command)
{
    if (command.listTargets.isEmpty()) {
        printf("Error: --showstructs requires <target>\n");
        return XOptions::CR_INVALIDPARAMETER;
    }

    XOptions::CR result = XOptions::CR_SUCCESS;
    const QString sFileName = command.listTargets.at(0);

    QFile file;
    file.setFileName(sFileName);

    if (file.open(QIODevice::ReadOnly)) {
        XBinary::PDSTRUCT pdStruct = XBinary::createPdStruct();
        XBinary::FT fileType = command.fileType;

        if (fileType == XBinary::FT_UNKNOWN) {
            fileType = XFormats::getPrefFileType(&file, XBinary::FT_FLAG_FORMATS, &pdStruct);
        }

        XBinary *pBinary = XFormats::createClass(fileType, &file);

        if (pBinary) {
            const QList<XBinary::XFHEADER> listHeaders = pBinary->_getXFHeaders(&pdStruct);
            XBinary::INDATA inData = XFormats::createINDATA(fileType, &file);

            XFTreeModel treeModel(nullptr);
            treeModel.setData(inData, listHeaders);

            QString sStructs;

            if (command.resultFormat == RESULTFORMAT_JSON) sStructs = treeModel.toJSON();
            else if (command.resultFormat == RESULTFORMAT_XML) sStructs = treeModel.toXML();
            else if (command.resultFormat == RESULTFORMAT_CSV) sStructs = treeModel.toCSV();
            else if (command.resultFormat == RESULTFORMAT_TSV) sStructs = treeModel.toTSV();
            else sStructs = treeModel.toFormattedString();

            printf("%s", sStructs.toUtf8().data());

            delete pBinary;
        } else {
            printf("Cannot read structures: %s\n", sFileName.toUtf8().data());
            result = XOptions::CR_CANNOTOPENFILE;
        }

        file.close();
    } else {
        printf("Cannot open: %s\n", sFileName.toUtf8().data());
        result = XOptions::CR_CANNOTOPENFILE;
    }

    return result;
}

// ---------------------------------------------------------------------------
// Standalone console entry point
// ---------------------------------------------------------------------------

int XArchiveConsole::process(QCoreApplication &app, const QString &sDescription)
{
    // Text codecs (cp437 for DOS-era archive names, the legacy ANSI code pages
    // an installer may use) are needed by the format parsers.
    XOptions::registerCodecs();

    // 7-Zip and Info-ZIP command lines are answered first: their clustered and
    // attached-value switch forms are not expressible as Qt options, and
    // QCommandLineParser::process() would reject them outright.
    qint32 nDialectResult = XOptions::CR_SUCCESS;

    if (processForeignDialect(app.arguments(), &nDialectResult)) {
        return nDialectResult;
    }

    QCommandLineParser parser;
    parser.setApplicationDescription(sDescription);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("target", "The file or directory to open.");

    // addVersionOption() owns -v, so verbosity uses -b; addOptionChecked()
    // guarantees a letter that is already taken can never be dropped silently.
    if (!addOptions(&parser)) {
        return XOptions::CR_INVALIDPARAMETER;
    }

    parser.process(app);

    XOptions::CR crResult = XOptions::CR_SUCCESS;

    if (!applyOptions(&parser, &crResult)) {
        return crResult;
    }

    if (parser.isSet(m_clNoColor)) {
        XOptions::setNoColor(true);
    }

    COMMAND command;

    if (!buildCommand(&parser, &command, &crResult)) {
        return crResult;
    }

    if (command.verb == VERB_NONE) {
        parser.showHelp();
        Q_UNREACHABLE();
    }

    qint32 nResult = execute(command);

    if (m_bProbeTimeoutOccurred) {
        nResult = XOptions::CR_PROBETIMEOUT;
    }

    return nResult;
}
