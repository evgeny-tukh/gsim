#pragma once

byte calcCRC (char *);

#include "sim_types.h"
#include "target.h"

#define _101000B_           0x28
#define _110000B_           0x30
#define _111000B_           0x38
#define _100000B_           0x20
#define _10000000B_         0x80

namespace AIS
{
    byte *buildStaticShipAndVoyageData (byte *buffer, Target *target, const int retransmitCount = 0);
    byte *buildPositionReport (byte *buffer, Target *target, const int retransmitCount = 0);
    byte *buildSafeAndResqueReport (byte *buffer, Target *target, const int retransmitCount = 0);
    byte *buildBroadcastSRM (byte *buffer, Target *target, const int retransmitCount = 0);

    unsigned char calcCRC (char *sentence)
    {
        unsigned char result = 0;

        if (sentence)
        {
            result = sentence [1];

            for (int i = 2; sentence [i] && sentence [i] != '*'; ++ i)
                result ^= sentence [i];
        }

        return result;
    }

    inline word swapBytes (const word data)
    {
        word  result;
        byte *source = (byte *) & data;
        byte *dest   = (byte *) & result;

        dest [0] = source [1];
        dest [1] = source [0];

        return result;
    }

    inline dword swapBytes (const dword data)
    {
        dword result;
        byte *source = (byte *) & data;
        byte *dest   = (byte *) & result;

        dest [0] = source [3];
        dest [1] = source [2];
        dest [2] = source [1];
        dest [3] = source [0];

        return result;
    }

    inline byte sixBit2Ascii (const byte sixBit)
    {
        return (sixBit < _101000B_) ? (sixBit + _110000B_) : (sixBit + _111000B_);
    }

    void encodeString (byte *buffer, char *charBuffer, const int size);

    bool getBit (byte *buffer, const int bitNo);
    void setBit (byte *buffer, const int bitNo, const bool set);

    void encodeAndStore (byte *destBuffer, const int destBit, byte *source, const int startSourceBit, const int count);
    void smartEncodeAndStore (byte *destBuffer, int& destBit, char *source, const int sourceSize);
    void smartEncodeAndStore (byte *destBuffer, int& destBit, byte *source, const int sourceSize, const int count);
    void smartEncodeAndStore (byte *destBuffer, int& destBit, byte source, const int count);
    void smartEncodeAndStore (byte *destBuffer, int& destBit, word source, const int count);
    void smartEncodeAndStore (byte *destBuffer, int& destBit, dword source, const int count);

    void buildVDM (const char *, std::strings&, const size_t, const int fillBits = 0);

    const int buildImoNumber (const int id)
    {
        int digits [7], i, result, factor;

        for (i = 0, digits [6] = 0; i < 6; ++ i)
        {
            digits [i]  = 9 - i;
            digits [6] += digits [i] * (7 - i);
        }

        digits [6] = digits [6] % 10;

        for (i = result = 0, factor = 1e6; i < 7; ++ i, factor /= 10)
            result += digits [i] * factor;

        return result;
    }
}

void AIS::buildVDM (const char *data, std::strings& sentences, const size_t size, const int fillBits)
{
    if (data)
    {
        char        format [300];
        char        buffer [83];
        static byte seqNo = 0;
        size_t      size  = strlen (data);

        if (size <= 63)
        {
            // Put all the data to the single sentence
            sprintf (format, "!AIVDM,1,1,%d,A,%s,%d*%%02X\r\n", (seqNo ++) % 10, data, fillBits);
            sprintf (buffer, format, calcCRC (format));

            sentences.push_back (buffer);
        }
        else
        {
            // Split data
            char dataPart [63];
            int  i, j, k;

            for (i = 0, j = 1, k = size / 62 + ((size % 62) ? 1 : 0); i < size; i += 62, ++ j)
            {
                memset (dataPart, 0, sizeof (dataPart));
                strncpy (dataPart, data + i, sizeof (dataPart) - 1);
                sprintf (format, "!AIVDM,%d,%d,%d,A,%s,%d*%%02X\r\n",
                         k, j, seqNo % 10, dataPart, i < (size - 1) ? 0 : fillBits);
                sprintf (buffer, format, calcCRC (format));

                sentences.push_back (buffer);
            }
        }

        ++ seqNo;
    }
}

void AIS::encodeAndStore (byte *destBuffer, const int destBit, byte *source, const int startSourceBit, const int count)
{
    int i;

    for (i = 0; i < count; ++ i)
        setBit (destBuffer, destBit + i, getBit (source, startSourceBit + i));
}

void AIS::smartEncodeAndStore (byte *destBuffer, int& destBit, char *source, const int sourceSize)
{
    int  i,
         j;
    byte dataChar;

    for (i = 0, j = destBit; i < sourceSize; ++ i, j += 6)
    {
        dataChar = toupper (source [i]);

        if (dataChar >= 0x40 && dataChar <= 0x60)
            dataChar -= 0x40;

        encodeAndStore (destBuffer, j, & dataChar, 2, 6);
    }

    destBit += sourceSize * 6;
}

void AIS::smartEncodeAndStore (byte *destBuffer, int& destBit, byte *source, const int sourceSize, const int count)
{
    encodeAndStore (destBuffer, destBit, source, (sourceSize << 3) - count, count);

    destBit += count;
}

void AIS::smartEncodeAndStore (byte *destBuffer, int& destBit, byte source, const int count)
{
    encodeAndStore (destBuffer, destBit, & source, (sizeof (byte) << 3) - count, count);

    destBit += count;
}

void AIS::smartEncodeAndStore (byte *destBuffer, int& destBit, word source, const int count)
{
    source = swapBytes (source);

    encodeAndStore (destBuffer, destBit, (byte *) & source, (sizeof (word) << 3) - count, count);

    destBit += count;
}

void AIS::smartEncodeAndStore (byte *destBuffer, int& destBit, dword source, const int count)
{
    source = swapBytes (source);

    encodeAndStore (destBuffer, destBit, (byte *) & source, (sizeof (dword) << 3) - count, count);

    destBit += count;
}

bool AIS::getBit (byte *buffer, const int bitNo)
{
    int  byteNo    = (bitNo >> 3),
         bitInByte = bitNo - (byteNo << 3);                      // From left to right
    byte dataByte  = buffer [byteNo],
         mask      = (0x80 >> bitInByte);

    return (dataByte & mask) != 0;
}

void AIS::setBit (byte *buffer, const int bitNo, const bool set)
{
    int  byteNo     = (bitNo >> 3),
         bitInByte = bitNo - (byteNo << 3),                      // From left to right
         mask      = (0x80 >> bitInByte);

    if (set)
         buffer [byteNo] |= mask;
    else if (buffer [byteNo] & mask)
         buffer [byteNo] ^= mask;
}

void AIS::encodeString (byte *buffer, char *charBuffer, const int size)
{
    byte data;
    int  i,
         j;

    for (i = 0; i < size; ++ i)
    {
        data = 0;

        for (j = 0; j < 6; ++ j)
            setBit (& data, j + 2, getBit ((byte *) buffer, i * 6 + j));

        charBuffer [i] = (char) sixBit2Ascii (data);
    }
}


byte *AIS::buildStaticShipAndVoyageData (byte *buffer, Target *target, const int retransmitCount)
{
    int count = 0;

    smartEncodeAndStore (buffer, count, (byte) 5, 6);
    smartEncodeAndStore (buffer, count, (byte) retransmitCount, 2);
    smartEncodeAndStore (buffer, count, (dword) target->id, 30);
    smartEncodeAndStore (buffer, count, (byte) 0, 2);
    smartEncodeAndStore (buffer, count, (dword) target->imo, 30);
    smartEncodeAndStore (buffer, count, "@@@@@@@", 7);
    smartEncodeAndStore (buffer, count, target->name, 20);
    smartEncodeAndStore (buffer, count, (byte) 0, 8);
    smartEncodeAndStore (buffer, count, (word) 0, 9);
    smartEncodeAndStore (buffer, count, (word) 0, 9);
    smartEncodeAndStore (buffer, count, (byte) 0, 6);
    smartEncodeAndStore (buffer, count, (byte) 0, 6);
    smartEncodeAndStore (buffer, count, (byte) 0, 4);
    smartEncodeAndStore (buffer, count, (byte) 0, 4);
    smartEncodeAndStore (buffer, count, (byte) 0, 5);
    smartEncodeAndStore (buffer, count, (byte) 0, 5);
    smartEncodeAndStore (buffer, count, (byte) 0, 6);
    smartEncodeAndStore (buffer, count, (byte) 0, 8);
    smartEncodeAndStore (buffer, count, "@@@@@@@@@@@@@@@@@@@@", 20);
    smartEncodeAndStore (buffer, count, (byte) 0, 1);
    smartEncodeAndStore (buffer, count, (byte) 0, 1);

    return buffer;
}

byte *AIS::buildBroadcastSRM (byte *buffer, Target *target, const int retransmitCount)
{
    int count = 0;

    smartEncodeAndStore (buffer, count, (byte) 14, 6);
    smartEncodeAndStore (buffer, count, (byte) retransmitCount, 2);
    smartEncodeAndStore (buffer, count, (dword) target->id, 30);
    smartEncodeAndStore (buffer, count, (byte) 0, 2);
    smartEncodeAndStore (buffer, count, "ACTIVE SART", 11);

    return buffer;
}

byte *AIS::buildPositionReport (byte *buffer, Target *target, const int retransmitCount)
{
    int count = 0;

    smartEncodeAndStore (buffer, count, (byte) 1, 6);
    smartEncodeAndStore (buffer, count, (byte) retransmitCount, 2);
    smartEncodeAndStore (buffer, count, (dword) target->id, 30);
    smartEncodeAndStore (buffer, count, (byte) (target->sog > 0.0 ? 0 : 1), 4);
    smartEncodeAndStore (buffer, count, (byte) 0, 8);
    smartEncodeAndStore (buffer, count, (word) (target->sog * 10.0f), 10);
    smartEncodeAndStore (buffer, count, (byte) 1, 1);
    smartEncodeAndStore (buffer, count, (dword) (target->lon * 600000.0), 28);
    smartEncodeAndStore (buffer, count, (dword) (target->lat * 600000.0), 27);
    smartEncodeAndStore (buffer, count, (word) (target->cog * 10.0f), 12);
    smartEncodeAndStore (buffer, count, (word) target->cog, 9);
    smartEncodeAndStore (buffer, count, (byte) 0, 6);
    smartEncodeAndStore (buffer, count, (byte) 0, 4);
    smartEncodeAndStore (buffer, count, (byte) 0, 1);
    smartEncodeAndStore (buffer, count, (byte) 0, 1);
    smartEncodeAndStore (buffer, count, (dword) 0, 19);

    return buffer;
}

byte *AIS::buildSafeAndResqueReport (byte *buffer, Target *target, const int retransmitCount)
{
    int count = 0;

    smartEncodeAndStore (buffer, count, (byte) 9, 6);
    smartEncodeAndStore (buffer, count, (byte) retransmitCount, 2);
    smartEncodeAndStore (buffer, count, (dword) target->id, 30);
    smartEncodeAndStore (buffer, count, (word) (target->alt > 0 ? target->alt : 4095), 12);
    smartEncodeAndStore (buffer, count, (word) (target->sog > 0.1f ? target->sog : 1023), 10);
    smartEncodeAndStore (buffer, count, (byte) 1, 1);
    smartEncodeAndStore (buffer, count, (dword) (target->lon * 600000.0), 28);
    smartEncodeAndStore (buffer, count, (dword) (target->lat * 600000.0), 27);
    smartEncodeAndStore (buffer, count, (word) (target->cog * 10.0f), 12);
    smartEncodeAndStore (buffer, count, (word) target->cog, 9);
    smartEncodeAndStore (buffer, count, (byte) 0, 6);
    smartEncodeAndStore (buffer, count, (byte) 0, 8);
    smartEncodeAndStore (buffer, count, (byte) 1, 1);
    smartEncodeAndStore (buffer, count, (byte) 0, 5);
    smartEncodeAndStore (buffer, count, (byte) 0, 1);
    smartEncodeAndStore (buffer, count, (dword) 0, 19);

    return buffer;
}
