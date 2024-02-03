#include "Bitmap.hpp"

#include <std/math.h>

namespace std
{
    Bitmap::Bitmap(uint8_t* _bitmap, size_t bitcount, bool defaultValue) : bitmap(_bitmap)
    {
        bitmap_size = DivRoundUp(bitcount, 8);
        bitmap_count = bitcount;
        for(size_t i = 0; i < bitmap_size; i++)
        {
            bitmap[i] = 0xFF * (uint8_t)defaultValue;
        }
    }

    bool Bitmap::get(size_t bitIndex)
    {
        size_t byteIndex = bitIndex / 8;
        size_t byteOffset = bitIndex % 8;

        return (bitmap[byteIndex] >> byteOffset) & 0x1;
    }
    void Bitmap::set(size_t bitIndex, bool value)
    {
        size_t byteIndex = bitIndex / 8;
        size_t byteOffset = bitIndex % 8;

        if(value) bitmap[byteIndex] |= (1 << byteOffset);
        else bitmap[byteIndex] &= ~(1 << byteOffset);
    }

    size_t Bitmap::find_false()
    {
        if(cachedFalseBit)
        {
            return cached_FalseBit;
            cachedFalseBit = false;
        }

        for(size_t i = 0; i < bitmap_size; i++)
        {
            uint8_t value = bitmap[i];

            // if the byte is not equal to 0xFF
            // impiles that this byte contains a false value
            if(value != 0xFF)
            {
                for(uint8_t offset = 0; offset < 8; offset++)
                {
                    // if the value at that offset is false
                    // the >> will givew a even number
                    if((value >> offset) % 2 == 0)
                    {
                        // this is our value
                        return i * 8 + offset;
                    }
                }
            }
        }

        // not found
        return Bitmap::npos;
    }

    size_t Bitmap::find_false_bits(size_t n, bool cache)
    {
        cachedFalseBit = cache;

        for(size_t i = 0; i < bitmap_size; i++)
        {
            uint8_t value = bitmap[i];

            // if the byte is not equal to 0xFF
            // impiles that this byte contains a false value
            if(value != 0xFF)
            {
                for(uint8_t offset = 0; offset < 8; offset++)
                {
                    // if the value at that offset is false
                    // the >> will givew a even number
                    if((value >> offset) % 2 == 0)
                    {
                        if(cache)
                        {
                            cached_FalseBit = i * 8 + offset;
                            cache = false;
                        }

                        size_t loc = i * 8 + offset;

                        for(size_t j = 0; j < n; j++)
                        {
                            // not a false byte
                            // skip
                            if(get(loc + j))
                            {
                                goto nextByte;
                            }
                        }

                        return loc;
                    }
                }
            }

            nextByte:
            ;
        }

        cached_FalseBit = Bitmap::npos;

        return Bitmap::npos;
    }

    void Bitmap::setBits(size_t first, size_t size, bool value)
    {
        if(size < 8)
        {
            for(size_t i = 0; i < size; i++)
            {
                set(first + i, value);
            }

            return;
        }

        uint8_t byteValue = 0xFF * (uint8_t)value;
        size_t byteCount = size / 8;

        size_t firstByte = first / 8;
        size_t lastByte = (first + size) / 8;

        uint8_t beginOffset = first % 8;
        uint8_t lastOffset = 8 - ((first + size) % 8);

        // perfectly divisible then last byte is 1 before calculated value
        if(lastOffset == 8)
        {
            lastByte--;
            lastOffset = 0;
        }

        bitmap[firstByte] = (bitmap[firstByte] & ~(0xFF << beginOffset)) | (byteValue << beginOffset);

        //if(value) bitmap[firstByte] |= 0xFF << beginOffset;
        //else bitmap[firstByte] &= 0xFF >> (8 - beginOffset);

        //bitmap[firstByte] |= byteValue << beginOffset;

        for(size_t i = firstByte + 1; i < lastByte; i++)
        {
            bitmap[i] = byteValue;
        }

        bitmap[lastByte] = (bitmap[lastByte] & ~(0xFF >> lastOffset)) | (byteValue >> lastOffset);
    }

    size_t Bitmap::size()
    {
        return bitmap_count;
    }
}