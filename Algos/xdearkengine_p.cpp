/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 *
 * The private decoding algorithms used here are adapted from Deark 1.7.3-1,
 * Copyright (C) 2016-2026 Jason Summers. See xdearkdecoder.LICENSE.
 */
#include "xdearkengine_p.h"

#include <QFile>

#include <cstring>

#include "xdearkengine_private_p.h"
#include "xdearkengine_api_p.h"
#include "xdearkmoduleregistry_p.h"

namespace {

const char *const SUPPORTED_MODULES =
    "arcv,cazip,dclimplode,edi_pack,os2pack,os2pack2,graspgl,gxlib,"
    "tscomp,is_z,is_instarch,mdcd,mrnz,pcshrink,pcxlib,packit,lha,"
    "red,loaddskf,stuffit,spark,squeeze,cpshrink,lif_kdc,nufx,"
    "zlib,dskexp,wizsolitaire,rsc,compress_lzh";

struct EngineResult {
    int runResult;
    int fatalError;
    int errorCount;
    int extractedFiles;
    int64_t totalOutputSize;
    char moduleName[80];
    char errorMessage[1024];
};

struct EngineContext {
    EngineResult *result;
};

class XDearkFatalError final
{
};

class XDearkSession final
{
public:
    explicit XDearkSession(EngineContext *engineContext)
        : m_context(XDearkModuleRegistry::createContext())
    {
        if (!m_context) return;
        de_set_userdata(m_context, engineContext);
        de_set_fatalerror_callback(m_context, fatalCallback);
        de_set_messages_callback(m_context, messageCallback);
    }

    ~XDearkSession()
    {
        if (m_context) de_destroy(m_context);
    }

    deark *context() const { return m_context; }

    XDearkSession(const XDearkSession &) = delete;
    XDearkSession &operator=(const XDearkSession &) = delete;

private:
    static void messageCallback(deark *context, UI flags,
                                const char *message);
    static void fatalCallback(deark *context);

    deark *m_context;
};

void appendMessage(char *destination, size_t destinationSize,
                   const char *source)
{
    if (!destination || destinationSize < 2 || !source || !source[0]) return;
    size_t used = std::strlen(destination);
    if (used >= destinationSize - 1) return;
    if (used) {
        destination[used++] = ';';
        if (used < destinationSize - 1) destination[used++] = ' ';
    }
    const size_t available = destinationSize - used - 1;
    if (available) std::strncat(destination, source, available);
}

void XDearkSession::messageCallback(deark *context, UI flags,
                                    const char *message)
{
    EngineContext *engine =
        static_cast<EngineContext *>(de_get_userdata(context));
    const unsigned int messageType = flags & 0xffU;
    if (!engine || !engine->result || !message) return;
    if (!std::strncmp(message, "Module: ", 8)) {
        size_t length = std::strlen(message + 8);
        if (length >= sizeof(engine->result->moduleName))
            length = sizeof(engine->result->moduleName) - 1;
        std::memcpy(engine->result->moduleName, message + 8, length);
        engine->result->moduleName[length] = '\0';
    }
    if (messageType == DE_MSGTYPE_ERROR)
        appendMessage(engine->result->errorMessage,
                      sizeof(engine->result->errorMessage), message);
}

void XDearkSession::fatalCallback(deark *context)
{
    EngineContext *engine =
        static_cast<EngineContext *>(de_get_userdata(context));
    if (!engine || !engine->result) return;
    engine->result->fatalError = 1;
    throw XDearkFatalError();
}

}  // namespace

QStringList XDearkEngine::supportedModules()
{
    return QString::fromLatin1(SUPPORTED_MODULES)
        .split(QLatin1Char(','), Qt::SkipEmptyParts);
}

bool XDearkEngine::extractToZip(const QString &inputPath,
                                const QString &outputPath,
                                const LIMITS &limits, RESULT *result)
{
    if (!result) return false;
    *result = RESULT();
    if (inputPath.isEmpty() || outputPath.isEmpty() || limits.maxFiles <= 0 ||
        limits.maxFileSize <= 0 || limits.maxTotalSize <= 0) {
        result->errorMessage = QStringLiteral("Invalid legacy decoder arguments");
        return false;
    }

    const QByteArray inputName = QFile::encodeName(inputPath);
    const QByteArray outputName = QFile::encodeName(outputPath);
    EngineResult engineResult = {};
    EngineContext engineContext = {};
    engineContext.result = &engineResult;
    try {
        XDearkSession session(&engineContext);
        deark *context = session.context();
        if (!context) {
            appendMessage(engineResult.errorMessage,
                          sizeof(engineResult.errorMessage),
                          "Cannot create legacy decoder context");
        } else {
            de_set_std_option_int(context, DE_STDOPT_EXTRACT_LEVEL, 2);
            de_set_std_option_int(context, DE_STDOPT_WARNINGS, 1);
            de_set_std_option_int(context, DE_STDOPT_INFOMESSAGES, 1);
            de_set_std_option_int(context, DE_STDOPT_FILENAMES_FROM_FILE, 1);
            de_set_std_option_int(context, DE_STDOPT_OVERWRITE_MODE,
                                  DE_OVERWRITEMODE_NEVER);
            de_set_output_style(context, DE_OUTPUTSTYLE_ARCHIVE,
                                DE_ARCHIVEFMT_ZIP);
            de_set_output_archive_filename(context, nullptr,
                                           outputName.constData(), 0);
            de_set_max_output_files(context, limits.maxFiles);
            de_set_max_output_file_size(context, limits.maxFileSize);
            de_set_max_total_output_size(context, limits.maxTotalSize);
            de_set_ext_option(context, "loaddskf:toraw", "1");
            de_set_disable_mods(context, SUPPORTED_MODULES, 1);
            de_set_input_filename(context, inputName.constData(), 0);

            engineResult.runResult = de_run(context);
            engineResult.errorCount = context->error_count;
            engineResult.extractedFiles = context->num_files_extracted;
            engineResult.totalOutputSize = context->total_output_size;
        }
    } catch (const XDearkFatalError &) {
        // fatalCallback has already recorded the failure in engineResult.
    }

    result->runResult = engineResult.runResult != 0;
    result->fatalError = engineResult.fatalError != 0;
    result->errorCount = engineResult.errorCount;
    result->extractedFiles = engineResult.extractedFiles;
    result->totalOutputSize = engineResult.totalOutputSize;
    result->module = QString::fromLatin1(engineResult.moduleName);
    result->errorMessage = QString::fromUtf8(engineResult.errorMessage);
    return result->runResult && !result->fatalError &&
           result->extractedFiles > 0;
}
