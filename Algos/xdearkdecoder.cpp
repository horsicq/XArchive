/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xdearkdecoder.h"

#include "xdearkengine_p.h"

bool XDearkDecoder::extractToZip(const QString &inputPath,
                                 const QString &outputPath,
                                 const LIMITS &limits, RESULT *result)
{
    if (!result) return false;
    *result = RESULT();

    XDearkEngine::LIMITS engineLimits;
    engineLimits.maxFiles = limits.maxFiles;
    engineLimits.maxFileSize = limits.maxFileSize;
    engineLimits.maxTotalSize = limits.maxTotalSize;

    XDearkEngine::RESULT engineResult;
    const bool success = XDearkEngine::extractToZip(
        inputPath, outputPath, engineLimits, &engineResult);

    result->runResult = engineResult.runResult;
    result->fatalError = engineResult.fatalError;
    result->errorCount = engineResult.errorCount;
    result->extractedFiles = engineResult.extractedFiles;
    result->totalOutputSize = engineResult.totalOutputSize;
    result->module = engineResult.module;
    result->errorMessage = engineResult.errorMessage;
    return success;
}

bool XDearkDecoder::isSupportedModule(const QString &module)
{
    return !module.isEmpty() &&
           XDearkEngine::supportedModules().contains(module,
                                                      Qt::CaseSensitive);
}
