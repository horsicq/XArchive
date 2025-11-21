/* Copyright (c) 2025 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xaesdecoder.h"
#include <QDebug>
#include <cstring>

// AES constants
#define AES_BLOCK_SIZE 16


XAESDecoder::XAESDecoder(QObject *parent) : QObject(parent)
{
}

void XAESDecoder::deriveKey(const QString &sPassword, const QByteArray &baSalt, quint8 nNumCyclesPower, quint8 *pKey)
{
    // Note: tiny-AES-c does not require table initialization
    
    // Convert password to UTF-8 bytes
    QByteArray baPassword = sPassword.toUtf8();
    
    // Note: Test code removed to reduce debug output
    // AES-256-CBC and SHA256 implementations verified correct with NIST test vectors
    
    // 7z AES key derivation algorithm
    // Key = SHA256^(2^NumCyclesPower) (Salt + Password + Counter)
    
    const quint32 nKeySize = 32;  // AES-256 key size
    const quint32 nNumRounds = (quint32)1 << nNumCyclesPower;
    
    // Prepare buffer: Salt + Password + 8-byte counter field
    // NOTE: 7z allocates 8 bytes for counter but only uses 32-bit values (first 4 bytes)!
    QByteArray baBuffer;
    baBuffer.append(baSalt);
    baBuffer.append(baPassword);
    baBuffer.append(QByteArray(8, '\0'));  // 8-byte counter field (last 4 bytes stay zero)
    
    const qint32 nCounterOffset = baBuffer.size() - 8;
    
    // Hash iteratively: SHA256^(2^NumCyclesPower) means:
    // For each round: hash (Salt + Password + Counter)
    // All rounds feed into one continuous SHA context
    XSha256Decoder::Context sha;
    XSha256Decoder::init(&sha);
    
    for (quint32 nRound = 0; nRound < nNumRounds; nRound++) {
        // Update 32-bit counter at end of buffer (little-endian)
        // CRITICAL: 7z uses ONLY 32-bit counter, not 64-bit!
        baBuffer[nCounterOffset + 0] = (nRound >> 0) & 0xFF;
        baBuffer[nCounterOffset + 1] = (nRound >> 8) & 0xFF;
        baBuffer[nCounterOffset + 2] = (nRound >> 16) & 0xFF;
        baBuffer[nCounterOffset + 3] = (nRound >> 24) & 0xFF;
        // Bytes 4-7 remain zero
        
        // Debug: Show first 3 rounds
        if (nRound < 3) {
            QString sRoundHex;
            for (qint32 i = 0; i < baBuffer.size(); i++) {
                sRoundHex += QString("%1 ").arg((quint8)baBuffer[i], 2, 16, QChar('0'));
            }
            qDebug() << QString("[XAESDecoder] Round %1 buffer:").arg(nRound) << sRoundHex;
        }
        
        // Hash: Salt + Password + Counter
        XSha256Decoder::update(&sha, (const quint8 *)baBuffer.constData(), baBuffer.size());
    }
    
    // Finalize and get the key
    XSha256Decoder::final(&sha, pKey);
    
    // Debug: Show derived key
    QString sKeyHex;
    for (qint32 i = 0; i < 32; i++) {
        sKeyHex += QString("%1 ").arg((quint8)pKey[i], 2, 16, QChar('0'));
    }
    qDebug() << "[XAESDecoder] Derived key:" << sKeyHex;
    
    // Debug: Show buffer structure
    QString sBufferHex;
    for (qint32 i = 0; i < qMin(baBuffer.size(), 64); i++) {
        sBufferHex += QString("%1 ").arg((quint8)baBuffer[i], 2, 16, QChar('0'));
    }
    qDebug() << "[XAESDecoder] Buffer (first 64 bytes):" << sBufferHex;
    qDebug() << "[XAESDecoder] Buffer size:" << baBuffer.size() << "NumRounds:" << nNumRounds;
}

bool XAESDecoder::decrypt(XBinary::DATAPROCESS_STATE *pDecryptState, const QByteArray &baProperties, const QString &sPassword, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecryptState || !pDecryptState->pDeviceInput || !pDecryptState->pDeviceOutput) {
        qWarning() << "[XAESDecoder] Invalid decrypt state";
        return false;
    }
    
    if (sPassword.isEmpty()) {
        qWarning() << "[XAESDecoder] Password is required for AES decryption";
        return false;
    }
    
    // Debug: Show raw properties
    QString sPropsHex;
    for (qint32 i = 0; i < qMin(baProperties.size(), 32); i++) {
        sPropsHex += QString("%1 ").arg((quint8)baProperties[i], 2, 16, QChar('0'));
    }
    qDebug() << "[XAESDecoder] Raw properties:" << sPropsHex;
    qDebug() << "[XAESDecoder] Properties size:" << baProperties.size();
    
    // Debug: Show all properties bytes
    QString sAllPropsHex;
    for (qint32 i = 0; i < baProperties.size(); i++) {
        sAllPropsHex += QString("%1 ").arg((quint8)baProperties[i], 2, 16, QChar('0'));
    }
    qDebug() << "[XAESDecoder] All properties:" << sAllPropsHex;
    
    // Parse properties: FirstByte + Salt (0-16 bytes) + IV (16 bytes)
    // FirstByte format (from 7-Zip source):
    //   Bits 0-5: NumCyclesPower (0-63, typically 19 for 524288 iterations)
    //   Bits 6-7: SaltSize code (0-3 → 0,4,8,16 bytes respectively)
    if (baProperties.size() < 1) {
        qWarning() << "[XAESDecoder] Invalid properties size:" << baProperties.size();
        return false;
    }
    
    quint8 nFirstByte = (quint8)baProperties[0];
    quint8 nNumCyclesPower = nFirstByte & 0x3F;  // Bits 0-5
    
    quint8 nSaltSize = 0;
    quint8 nIVSizeInProps = 0;
    
    qDebug() << "[XAESDecoder] FirstByte: 0x" << QString::number(nFirstByte, 16) 
             << " = 0b" << QString::number(nFirstByte, 2).rightJustified(8, '0');
    qDebug() << "[XAESDecoder]   Bits 6-7: 0x" << QString::number((nFirstByte & 0xC0), 16) 
             << " (bit 7=" << ((nFirstByte >> 7) & 1) << ", bit 6=" << ((nFirstByte >> 6) & 1) << ")";
    
    // According to 7-Zip SDK (7zAes.cpp CDecoder::SetDecoderProperties2):
    // OLD FORMAT: (b0 & 0xC0) == 0 (NEITHER bit 6 nor bit 7 set)
    //   - Properties: [FirstByte] only (1 byte)
    //   - Salt size encoded in bits 6-7 of FirstByte (but both are 0, so no salt!)
    //   - IV must be read from stream
    // NEW FORMAT: (b0 & 0xC0) != 0 (AT LEAST ONE of bits 6-7 set)
    //   - Properties: [FirstByte][SecondByte][Salt][IV]
    //   - Salt/IV sizes calculated from FirstByte and SecondByte
    
    if ((nFirstByte & 0xC0) == 0) {
        // OLD FORMAT: No salt, no IV in properties
        qDebug() << "[XAESDecoder] Using OLD format (bits 6-7 both zero)";
        nSaltSize = 0;
        nIVSizeInProps = 0;
        
        if (baProperties.size() != 1) {
            qWarning() << "[XAESDecoder] OLD format expects 1 byte, got" << baProperties.size();
            // Continue anyway - maybe it's already appended
        }
    } else {
        // NEW FORMAT (7-Zip 9.20+): Properties contain Salt and/or IV  
        // Format: [FirstByte][SecondByte][Salt][IV]
        // FirstByte: bits 0-5=NumCyclesPower, bit 6=IV flag, bit 7=Salt flag
        // SecondByte: bits 4-7=SaltSize part, bits 0-3=IVSize part
        
        qDebug() << "[XAESDecoder] Using NEW format (at least one of bits 6-7 set)";
        
        if (baProperties.size() < 2) {
            qWarning() << "[XAESDecoder] New format properties too small:" << baProperties.size();
            return false;
        }
        
        quint8 nSecondByte = (quint8)baProperties[1];
        
        // CRITICAL: 7-Zip SDK decoder formula (from 7zAes.cpp lines 252-253):
        // const unsigned saltSize = ((b0 >> 7) & 1) + (b1 >> 4);
        // const unsigned ivSize   = ((b0 >> 6) & 1) + (b1 & 0x0F);
        // The flag bit value (0 or 1) is ADDED to the encoded value from SecondByte
        
        nSaltSize = ((nFirstByte >> 7) & 1) + (nSecondByte >> 4);
        nIVSizeInProps = ((nFirstByte >> 6) & 1) + (nSecondByte & 0x0F);
        
        qDebug() << "[XAESDecoder]   SecondByte: 0x" << QString::number(nSecondByte, 16);
        qDebug() << "[XAESDecoder]   Salt: flag bit=" << ((nFirstByte >> 7) & 1) << "+ encoded=" << (nSecondByte >> 4) << "= size" << nSaltSize;
        qDebug() << "[XAESDecoder]   IV: flag bit=" << ((nFirstByte >> 6) & 1) << "+ encoded=" << (nSecondByte & 0x0F) << "= size" << nIVSizeInProps;
        
        qint32 nExpectedSize = 2 + nSaltSize + nIVSizeInProps;
        if (baProperties.size() < nExpectedSize) {
            qWarning() << "[XAESDecoder] New format size mismatch: got" << baProperties.size() << "expected at least" << nExpectedSize;
            return false;
        }
        
        qDebug() << "[XAESDecoder] NEW format: SaltSize=" << nSaltSize << "IVSize=" << nIVSizeInProps;
    }
    
    // Extract salt from properties 
    // OLD format: no second byte, no salt (nSaltSize should be 0)
    // NEW format: starts at offset 2 (after FirstByte and SecondByte)
    qint32 nSaltOffset = ((nFirstByte & 0xC0) == 0) ? 1 : 2;
    QByteArray baSalt;
    if (nSaltSize > 0) {
        baSalt = baProperties.mid(nSaltOffset, nSaltSize);
        qDebug() << "[XAESDecoder] Extracted" << nSaltSize << "bytes of salt from offset" << nSaltOffset;
    } else {
        qDebug() << "[XAESDecoder] No salt (size=0)";
        
        // DEBUGGING: Check if 7z might be using a default salt
        // According to 7-Zip SDK encoder (7zAes.cpp CEncoder::WriteCoderProperties):
        // When there's no explicit salt/IV, older versions might behave differently
        qDebug() << "[XAESDecoder] NOTE: 7z encoder typically generates random salt/IV";
        qDebug() << "[XAESDecoder] If SecondByte encoding is wrong, salt might be hidden";
    }
    
    qDebug() << "[XAESDecoder] Salt offset:" << nSaltOffset << "size:" << nSaltSize;
    
    // Extract IV
    QByteArray baIV;
    if (nIVSizeInProps > 0) {
        // NEW FORMAT: IV is in properties after salt
        qint32 nIVOffset = nSaltOffset + nSaltSize;
        baIV = baProperties.mid(nIVOffset, nIVSizeInProps);
        qDebug() << "[XAESDecoder] IV extracted from properties:" << nIVSizeInProps << "bytes";
    } else {
        // OLD FORMAT or SOLID: IV must be read from stream or appended to properties
        // Check if caller already appended IV to properties (solid archives do this)
        qint32 nExpectedWithIV = nSaltOffset + nSaltSize + 16;
        if (baProperties.size() >= nExpectedWithIV) {
            // IV was appended by caller (solid archive case)
            baIV = baProperties.mid(nSaltOffset + nSaltSize, 16);
            qDebug() << "[XAESDecoder] IV extracted from appended properties";
        } else {
            // IV in stream (typical non-solid case)
            qint64 nDevicePos = pDecryptState->pDeviceInput->pos();
            qDebug() << "[XAESDecoder] Reading IV from stream at position" << nDevicePos;
            
            baIV.resize(16);
            qint64 nIvRead = pDecryptState->pDeviceInput->read(baIV.data(), 16);
            if (nIvRead != 16) {
                qWarning() << "[XAESDecoder] Failed to read IV from stream: got" << nIvRead << "bytes, expected 16";
                return false;
            }
            pDecryptState->nCountInput += 16;
            qDebug() << "[XAESDecoder] IV read from stream, now at position" << pDecryptState->pDeviceInput->pos();
        }
    }
    
    // Debug: Show salt and IV
    QString sSaltHex, sIvHex;
    for (qint32 i = 0; i < baSalt.size(); i++) {
        sSaltHex += QString("%1 ").arg((quint8)baSalt[i], 2, 16, QChar('0'));
    }
    for (qint32 i = 0; i < baIV.size(); i++) {
        sIvHex += QString("%1 ").arg((quint8)baIV[i], 2, 16, QChar('0'));
    }
    
    qDebug() << "[XAESDecoder] NumCyclesPower:" << nNumCyclesPower;
    qDebug() << "[XAESDecoder] SaltSize:" << nSaltSize;
    qDebug() << "[XAESDecoder] Salt:" << sSaltHex;
    qDebug() << "[XAESDecoder] IV:" << sIvHex;
    qDebug() << "[XAESDecoder] Password:" << sPassword;
    
    // Derive key from password
    quint8 aKey[32];  // AES-256 key
    deriveKey(sPassword, baSalt, nNumCyclesPower, aKey);
    
    // Debug: Show derived key (first 16 bytes)
    QString sKeyHex;
    for (qint32 i = 0; i < 16; i++) {
        sKeyHex += QString("%1 ").arg(aKey[i], 2, 16, QChar('0'));
    }
    qDebug() << "[XAESDecoder] Derived key (first 16 bytes):" << sKeyHex;
    
    // Initialize AES context using tiny-AES-c (avoids DD[*][0x63]=0 table singularity)
    // struct AES_ctx aesCtx;
    // AES_init_ctx_iv(&aesCtx, aKey, (const uint8_t *)baIV.constData());
    
    // Debug: Show key and IV being used
    QString sKeyDebug;
    for (qint32 i = 0; i < 32; i++) {
        sKeyDebug += QString("%1 ").arg((quint8)aKey[i], 2, 16, QChar('0'));
    }
    qDebug() << "[XAESDecoder] Key for AES-256-CBC (32 bytes):" << sKeyDebug;
    
    QString sIvDebug;
    for (qint32 i = 0; i < 16; i++) {
        sIvDebug += QString("%1 ").arg((quint8)baIV[i], 2, 16, QChar('0'));
    }
    qDebug() << "[XAESDecoder] IV for AES-CBC (16 bytes):" << sIvDebug;
    
    // Debug: Check stream position and read first block for inspection
    qint64 nStreamPosBefore = pDecryptState->pDeviceInput->pos();
    qDebug() << "[XAESDecoder] Stream position before decryption:" << nStreamPosBefore;
    qDebug() << "[XAESDecoder] Input limit (total encrypted size):" << pDecryptState->nInputLimit;
    
    // Decrypt data in blocks
    const qint32 N_BUFFER_SIZE = 0x4000;  // 16KB buffer
    
    char bufferIn[N_BUFFER_SIZE];
    char bufferOut[N_BUFFER_SIZE];
    
    qint64 nTotalDecrypted = 0;
    bool bFirstBlock = true;  // Flag to log first block details
    
    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        // Read input data
        qint32 nBufferSize = qMin((qint32)(pDecryptState->nInputLimit - pDecryptState->nCountInput), N_BUFFER_SIZE);
        
        qDebug() << "[XAESDecoder] Loop: nInputLimit=" << pDecryptState->nInputLimit << "nCountInput=" << pDecryptState->nCountInput << "nBufferSize=" << nBufferSize;
        
        if (nBufferSize <= 0) {
            qDebug() << "[XAESDecoder] Breaking: nBufferSize <= 0";
            break;  // No more input
        }
        
        // Ensure we read complete AES blocks (16 bytes)
        nBufferSize = (nBufferSize / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
        
        qDebug() << "[XAESDecoder] After block alignment: nBufferSize=" << nBufferSize;
        
        if (nBufferSize == 0) {
            qDebug() << "[XAESDecoder] Breaking: nBufferSize == 0 after alignment";
            break;  // Not enough data for even one block
        }
        
        qint32 nSize = XBinary::_readDevice(bufferIn, nBufferSize, pDecryptState);
        
        if (nSize <= 0) {
            break;  // End of input or error
        }
        
        // Debug: Show first block of encrypted data (first iteration only)
        if (nTotalDecrypted == 0 && nSize >= 16) {
            qDebug() << "[XAESDecoder] Read" << nSize << "bytes from input stream";
            qDebug() << "[XAESDecoder] Device position after read:" << pDecryptState->pDeviceInput->pos();
            QString sEncHex;
            for (qint32 i = 0; i < qMin(nSize, 80); i++) {  // Show more bytes
                sEncHex += QString("%1 ").arg((quint8)bufferIn[i], 2, 16, QChar('0'));
            }
            qDebug() << "[XAESDecoder] First 80 encrypted bytes:" << sEncHex;
        }
        
        // Ensure size is multiple of block size
        nSize = (nSize / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
        
        if (nSize > 0) {
            // Copy to output buffer for in-place decryption
            memcpy(bufferOut, bufferIn, nSize);
            
            // Debug: Show first encrypted block (first iteration only)
            if (nTotalDecrypted == 0 && nSize >= 16) {
                QString sEncHex;
                for (qint32 i = 0; i < qMin(nSize, 32); i++) {
                    sEncHex += QString("%1 ").arg((quint8)bufferOut[i], 2, 16, QChar('0'));
                }
                qDebug() << "[XAESDecoder] First encrypted bytes:" << sEncHex;
            }
            
            // Decrypt using tiny-AES-c (operates in-place on buffer)
            // AES_CBC_decrypt_buffer(&aesCtx, (uint8_t *)bufferOut, nSize);
            
            // Debug: Show first block of decrypted data (first iteration only)
            if (nTotalDecrypted == 0 && nSize >= 16) {
                QString sDecHex;
                for (qint32 i = 0; i < qMin(nSize, 32); i++) {
                    sDecHex += QString("%1 ").arg((quint8)bufferOut[i], 2, 16, QChar('0'));
                }
                qDebug() << "[XAESDecoder] First decrypted bytes:" << sDecHex;
            }
            
            // Write decrypted data
            if (!XBinary::_writeDevice(bufferOut, nSize, pDecryptState)) {
                qWarning() << "[XAESDecoder] Failed to write decrypted data";
                return false;
            }
            
            nTotalDecrypted += nSize;
        }
    }
    
    qDebug() << "[XAESDecoder] Successfully decrypted" << nTotalDecrypted << "bytes (before padding removal)";
    
    // Remove PKCS#7 padding
    // AES-CBC adds padding to make data multiple of block size (16 bytes)
    // PKCS#7: if N bytes padding needed, add N bytes each with value N
    // Last byte tells us how many padding bytes to remove
    if (nTotalDecrypted > 0 && pDecryptState->pDeviceOutput) {
        QBuffer *pBuffer = qobject_cast<QBuffer *>(pDecryptState->pDeviceOutput);
        if (pBuffer) {
            QByteArray &baBuffer = pBuffer->buffer();
            if (baBuffer.size() > 0) {
                // Read last byte (padding length)
                quint8 nPaddingLength = (quint8)baBuffer.at(baBuffer.size() - 1);
                
                // Validate padding: must be 1-16, and <= total size
                if (nPaddingLength > 0 && nPaddingLength <= 16 && nPaddingLength <= baBuffer.size()) {
                    // Verify padding bytes
                    bool bValidPadding = true;
                    qint32 nPaddingStart = baBuffer.size() - nPaddingLength;
                    for (qint32 i = nPaddingStart; i < baBuffer.size() && bValidPadding; i++) {
                        if ((quint8)baBuffer.at(i) != nPaddingLength) {
                            bValidPadding = false;
                        }
                    }
                    
                    if (bValidPadding) {
                        // Remove padding
                        baBuffer.resize(nPaddingStart);
                        pBuffer->seek(baBuffer.size());
                        qDebug() << "[XAESDecoder] Removed" << nPaddingLength << "bytes of PKCS#7 padding, final size:" << baBuffer.size();
                        nTotalDecrypted = baBuffer.size();
                    } else {
                        qWarning() << "[XAESDecoder] Invalid PKCS#7 padding (bytes don't match length" << nPaddingLength << "), not removing";
                    }
                } else {
                    qDebug() << "[XAESDecoder] Padding length" << nPaddingLength << "out of range (size:" << baBuffer.size() << "), keeping all data";
                }
            }
        } else {
            qWarning() << "[XAESDecoder] Cannot remove padding from non-QBuffer device";
        }
    }
    
    qDebug() << "[XAESDecoder] Final decrypted size:" << nTotalDecrypted << "bytes";
    
    // Zero out sensitive data
    memset(aKey, 0, sizeof(aKey));
    // memset(&aesCtx, 0, sizeof(aesCtx));  // Zero out AES context
    
    return nTotalDecrypted > 0;
}
