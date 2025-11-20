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

// Complete AES implementation copied from 7z Aes.c (software-only, no hardware acceleration)
// This avoids the need to include inbox/C files directly

// Basic types
typedef unsigned char Byte;
typedef unsigned int UInt32;

// AES constants
#define AES_BLOCK_SIZE 16
#define AES_NUM_IVMRK_WORDS ((1 + 1 + 15) * 4)

// Local AES tables
static UInt32 T[256 * 4];
static UInt32 D[256 * 4];
static Byte InvS[256];

static const Byte Sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

// Helper macros and functions from 7z implementation
#define xtime(x) ((((x) << 1) ^ (((x) & 0x80) != 0 ? 0x1B : 0)) & 0xFF)
#define Ui32(a0, a1, a2, a3) ((UInt32)(a0) | ((UInt32)(a1) << 8) | ((UInt32)(a2) << 16) | ((UInt32)(a3) << 24))
#define gb0(x) ((x) & 0xFF)
#define gb1(x) (((x) >> 8) & 0xFF)
#define gb2(x) (((x) >> 16) & 0xFF)
#define gb3(x) ((x) >> 24)
#define gb(n, x) gb##n(x)
#define TT(x) (T + (x << 8))
#define DD(x) (D + (x << 8))

static inline UInt32 GetUi32(const Byte *p)
{
    return (UInt32)p[0] | ((UInt32)p[1] << 8) | ((UInt32)p[2] << 16) | ((UInt32)p[3] << 24);
}

static inline void SetUi32(Byte *p, UInt32 v)
{
    p[0] = (Byte)(v & 0xFF);
    p[1] = (Byte)((v >> 8) & 0xFF);
    p[2] = (Byte)((v >> 16) & 0xFF);
    p[3] = (Byte)((v >> 24) & 0xFF);
}

// AES tables initialization
void AesGenTables(void)
{
    unsigned i;
    for (i = 0; i < 256; i++) {
        InvS[Sbox[i]] = (Byte)i;
    }

    for (i = 0; i < 256; i++) {
        {
            const UInt32 a1 = Sbox[i];
            const UInt32 a2 = xtime(a1);
            const UInt32 a3 = a2 ^ a1;
            TT(0)[i] = Ui32(a2, a1, a1, a3);
            TT(1)[i] = Ui32(a3, a2, a1, a1);
            TT(2)[i] = Ui32(a1, a3, a2, a1);
            TT(3)[i] = Ui32(a1, a1, a3, a2);
        }
        {
            const UInt32 a1 = InvS[i];
            const UInt32 a2 = xtime(a1);
            const UInt32 a4 = xtime(a2);
            const UInt32 a8 = xtime(a4);
            const UInt32 a9 = a8 ^ a1;
            const UInt32 aB = a8 ^ a2 ^ a1;
            const UInt32 aD = a8 ^ a4 ^ a1;
            const UInt32 aE = a8 ^ a4 ^ a2;
            DD(0)[i] = Ui32(aE, a9, aD, aB);
            DD(1)[i] = Ui32(aB, aE, a9, aD);
            DD(2)[i] = Ui32(aD, aB, aE, a9);
            DD(3)[i] = Ui32(a9, aD, aB, aE);
        }
    }
}

// Macros for encryption/decryption rounds
#define HT(i, x, s) TT(x)[gb(x, s[(i + x) & 3])]
#define HT4(m, i, s, p) m[i] = HT(i, 0, s) ^ HT(i, 1, s) ^ HT(i, 2, s) ^ HT(i, 3, s) ^ w[p + i]
#define HT16(m, s, p) HT4(m, 0, s, p); HT4(m, 1, s, p); HT4(m, 2, s, p); HT4(m, 3, s, p);
#define FT(i, x) Sbox[gb(x, m[(i + x) & 3])]
#define FT4(i) dest[i] = Ui32(FT(i, 0), FT(i, 1), FT(i, 2), FT(i, 3)) ^ w[i];

#define HD(i, x, s) DD(x)[gb(x, s[(i - x) & 3])]
#define HD4(m, i, s, p) m[i] = HD(i, 0, s) ^ HD(i, 1, s) ^ HD(i, 2, s) ^ HD(i, 3, s) ^ w[p + i];
#define HD16(m, s, p) HD4(m, 0, s, p); HD4(m, 1, s, p); HD4(m, 2, s, p); HD4(m, 3, s, p);
#define FD(i, x) InvS[gb(x, m[(i - x) & 3])]
#define FD4(i) dest[i] = Ui32(FD(i, 0), FD(i, 1), FD(i, 2), FD(i, 3)) ^ w[i];

// Key expansion for encryption
static void Aes_SetKey_Enc(UInt32 *w, const Byte *key, unsigned keySize)
{
    unsigned i;
    unsigned m;
    const UInt32 *wLim;
    UInt32 t;
    UInt32 rcon = 1;

    keySize /= 4;
    w[0] = ((UInt32)keySize / 2) + 3;
    w += 4;

    for (i = 0; i < keySize; i++, key += 4) {
        w[i] = GetUi32(key);
    }

    t = w[(size_t)keySize - 1];
    wLim = w + (size_t)keySize * 3 + 28;
    m = 0;
    do {
        if (m == 0) {
            t = Ui32(Sbox[gb1(t)] ^ rcon, Sbox[gb2(t)], Sbox[gb3(t)], Sbox[gb0(t)]);
            rcon <<= 1;
            if (rcon & 0x100) {
                rcon = 0x1b;
            }
            m = keySize;
        } else if (m == 4 && keySize > 6) {
            t = Ui32(Sbox[gb0(t)], Sbox[gb1(t)], Sbox[gb2(t)], Sbox[gb3(t)]);
        }
        m--;
        t ^= w[0];
        w[keySize] = t;
    } while (++w != wLim);
}

// Key expansion for decryption
void Aes_SetKey_Dec(UInt32 *w, const Byte *key, unsigned keySize)
{
    unsigned i;
    unsigned num;
    Aes_SetKey_Enc(w, key, keySize);
    num = keySize + 20;
    w += 8;
    for (i = 0; i < num; i++) {
        UInt32 r = w[i];
        w[i] = DD(0)[Sbox[gb0(r)]] ^ DD(1)[Sbox[gb1(r)]] ^ DD(2)[Sbox[gb2(r)]] ^ DD(3)[Sbox[gb3(r)]];
    }
}

// Core AES decryption function
static inline void Aes_Decode(const UInt32 *w, UInt32 *dest, const UInt32 *src)
{
    UInt32 s[4];
    UInt32 m[4];
    UInt32 numRounds2 = w[0];
    w += 4 + numRounds2 * 8;
    s[0] = src[0] ^ w[0];
    s[1] = src[1] ^ w[1];
    s[2] = src[2] ^ w[2];
    s[3] = src[3] ^ w[3];
    for (;;) {
        w -= 8;
        HD16(m, s, 4)
        if (--numRounds2 == 0) {
            break;
        }
        HD16(s, m, 0)
    }
    FD4(0)
    FD4(1)
    FD4(2)
    FD4(3)
}

// CBC mode initialization
void AesCbc_Init(UInt32 *p, const Byte *iv)
{
    unsigned i;
    for (i = 0; i < 4; i++) {
        p[i] = GetUi32(iv + i * 4);
    }
}

// CBC mode decryption
void AesCbc_Decode(UInt32 *p, Byte *data, size_t numBlocks)
{
    UInt32 in[4];
    UInt32 out[4];
    for (; numBlocks != 0; numBlocks--, data += AES_BLOCK_SIZE) {
        in[0] = GetUi32(data);
        in[1] = GetUi32(data + 4);
        in[2] = GetUi32(data + 8);
        in[3] = GetUi32(data + 12);

        Aes_Decode(p + 4, out, in);

        SetUi32(data, p[0] ^ out[0]);
        SetUi32(data + 4, p[1] ^ out[1]);
        SetUi32(data + 8, p[2] ^ out[2]);
        SetUi32(data + 12, p[3] ^ out[3]);

        p[0] = in[0];
        p[1] = in[1];
        p[2] = in[2];
        p[3] = in[3];
    }
}

XAESDecoder::XAESDecoder(QObject *parent) : QObject(parent)
{
}

void XAESDecoder::deriveKey(const QString &sPassword, const QByteArray &baSalt, quint8 nNumCyclesPower, quint8 *pKey)
{
    // Convert password to UTF-8 bytes
    QByteArray baPassword = sPassword.toUtf8();
    
    // 7z AES key derivation algorithm
    // Key = SHA256^(2^NumCyclesPower) (Salt + Password + Counter)
    
    const quint32 nKeySize = 32;  // AES-256 key size
    const quint32 nNumRounds = (quint32)1 << nNumCyclesPower;
    
    // Prepare buffer: Salt + Password + 8-byte counter
    QByteArray baBuffer;
    baBuffer.append(baSalt);
    baBuffer.append(baPassword);
    baBuffer.append(QByteArray(8, '\0'));  // 8-byte counter initialized to 0
    
    // Initialize SHA256
    XSha256Decoder::Context sha;
    XSha256Decoder::init(&sha);
    
    // Hash iteratively
    for (quint32 nRound = 0; nRound < nNumRounds; nRound++) {
        // Update counter (last 8 bytes of buffer)
        quint32 nCounterLow = nRound;
        quint32 nCounterHigh = 0;
        
        qint32 nCounterOffset = baBuffer.size() - 8;
        baBuffer[nCounterOffset + 0] = (nCounterLow >> 0) & 0xFF;
        baBuffer[nCounterOffset + 1] = (nCounterLow >> 8) & 0xFF;
        baBuffer[nCounterOffset + 2] = (nCounterLow >> 16) & 0xFF;
        baBuffer[nCounterOffset + 3] = (nCounterLow >> 24) & 0xFF;
        baBuffer[nCounterOffset + 4] = (nCounterHigh >> 0) & 0xFF;
        baBuffer[nCounterOffset + 5] = (nCounterHigh >> 8) & 0xFF;
        baBuffer[nCounterOffset + 6] = (nCounterHigh >> 16) & 0xFF;
        baBuffer[nCounterOffset + 7] = (nCounterHigh >> 24) & 0xFF;
        
        // Update SHA256 with current buffer
        XSha256Decoder::update(&sha, (const quint8 *)baBuffer.constData(), baBuffer.size());
    }
    
    // Finalize and get the key
    XSha256Decoder::final(&sha, pKey);
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
    
    // Parse properties: NumCyclesPower (1 byte) + SaltSize (1 byte) + Salt (0-16 bytes) + IV (16 bytes)
    if (baProperties.size() < 2) {
        qWarning() << "[XAESDecoder] Invalid properties size:" << baProperties.size();
        return false;
    }
    
    quint8 nNumCyclesPower = (quint8)baProperties[0];
    quint8 nSaltSize = (quint8)baProperties[1];
    
    if (nSaltSize > 16) {
        qWarning() << "[XAESDecoder] Invalid salt size:" << nSaltSize;
        return false;
    }
    
    qint32 nMinPropertiesSize = 2 + nSaltSize + 16;  // Header + Salt + IV
    if (baProperties.size() < nMinPropertiesSize) {
        qWarning() << "[XAESDecoder] Properties too small:" << baProperties.size() << "expected at least" << nMinPropertiesSize;
        return false;
    }
    
    // Extract salt and IV
    QByteArray baSalt = baProperties.mid(2, nSaltSize);
    QByteArray baIV = baProperties.mid(2 + nSaltSize, 16);
    
    qDebug() << "[XAESDecoder] NumCyclesPower:" << nNumCyclesPower;
    qDebug() << "[XAESDecoder] SaltSize:" << nSaltSize;
    qDebug() << "[XAESDecoder] IV size:" << baIV.size();
    
    // Derive key from password
    quint8 aKey[32];  // AES-256 key
    deriveKey(sPassword, baSalt, nNumCyclesPower, aKey);
    
    // Initialize AES tables (call once)
    AesGenTables();
    
    // Set up AES decryption
    // ivAes array layout: IV (4 words) + keyMode (4 words) + roundKeys (15*4 words)
    UInt32 ivAes[AES_NUM_IVMRK_WORDS];
    
    // Set decryption key
    Aes_SetKey_Dec(ivAes + 4, aKey, 32);  // +4 to skip IV storage
    
    // Set IV
    AesCbc_Init(ivAes, (const Byte *)baIV.constData());
    
    // Decrypt data in blocks
    const qint32 N_BUFFER_SIZE = 0x4000;  // 16KB buffer
    
    char bufferIn[N_BUFFER_SIZE];
    char bufferOut[N_BUFFER_SIZE];
    
    qint64 nTotalDecrypted = 0;
    
    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        // Read input data
        qint32 nBufferSize = qMin((qint32)(pDecryptState->nInputLimit - pDecryptState->nCountInput), N_BUFFER_SIZE);
        
        if (nBufferSize <= 0) {
            break;  // No more input
        }
        
        // Ensure we read complete AES blocks (16 bytes)
        nBufferSize = (nBufferSize / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
        
        if (nBufferSize == 0) {
            break;  // Not enough data for even one block
        }
        
        qint32 nSize = XBinary::_readDevice(bufferIn, nBufferSize, pDecryptState);
        
        if (nSize <= 0) {
            break;  // End of input or error
        }
        
        // Ensure size is multiple of block size
        nSize = (nSize / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
        
        if (nSize > 0) {
            // Decrypt the data
            qint32 nNumBlocks = nSize / AES_BLOCK_SIZE;
            
            // Copy to output buffer for in-place decryption
            memcpy(bufferOut, bufferIn, nSize);
            
            // Decrypt using AES-CBC (generic implementation)
            AesCbc_Decode(ivAes, (Byte *)bufferOut, nNumBlocks);
            
            // Write decrypted data
            if (!XBinary::_writeDevice(bufferOut, nSize, pDecryptState)) {
                qWarning() << "[XAESDecoder] Failed to write decrypted data";
                return false;
            }
            
            nTotalDecrypted += nSize;
        }
    }
    
    qDebug() << "[XAESDecoder] Successfully decrypted" << nTotalDecrypted << "bytes";
    
    // Zero out sensitive data
    memset(aKey, 0, sizeof(aKey));
    memset(ivAes, 0, sizeof(ivAes));
    
    return nTotalDecrypted > 0;
}
