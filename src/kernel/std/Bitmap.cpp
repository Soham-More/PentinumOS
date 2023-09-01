#include "Bitmap.hpp"

#include <std/math.h>

namespace std
{
    Bitmap::Bitmap(uint8_t* _bitmap, uint32_t bitcount, bool defaultValue) : bitmap(_bitmap)
    {
        bitmap_size = DivRoundUp(bitcount, 8);
        bitmap_count = bitcount;
        for(uint32_t i = 0; i < bitmap_size; i++)
        {
            bitmap[i] = 0xFF * (uint8_t)defaultValue;
        }
    }

    bool Bitmap::get(uint32_t bitIndex)
    {
        uint32_t byteIndex = bitIndex / 8;
        uint32_t byteOffset = bitIndex % 8;

        return (bitmap[byteIndex] >> byteOffset) & 0x1;
    }
    void Bitmap::set(uint32_t bitIndex, bool value)
    {
        uint32_t byteIndex = bitIndex / 8;
        uint32_t byteOffset = bitIndex % 8;

        if(value) bitmap[byteIndex] |= (1 << byteOffset);
        else bitmap[byteIndex] &= ~(1 << byteOffset);
    }

    uint32_t Bitmap::find_false()
    {
        for(uint32_t i = 0; i < bitmap_size; i++)
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

    uint32_t Bitmap::find_false_bits(uint32_t n)
    {
        for(uint32_t i = 0; i < bitmap_size; i++)
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
                        uint32_t loc = i * 8 + offset;

                        for(uint32_t j = 0; j < n; j++)
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
        }

        return Bitmap::npos;
    }

    void Bitmap::setBits(uint32_t first, uint32_t size, bool value)
    {
        if(size < 8)
        {
            for(uint32_t i = 0; i < size; i++)
            {
                set(first + i, value);
            }

            return;
        }

        uint8_t byteValue = 0xFF * (uint8_t)value;
        uint32_t byteCount = size / 8;

        uint32_t firstByte = first / 8;
        uint32_t lastByte = (first + size) / 8;

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

        for(uint32_t i = firstByte + 1; i < lastByte; i++)
        {
            bitmap[i] = byteValue;
        }

        bitmap[lastByte] = (bitmap[lastByte] & ~(0xFF >> lastOffset)) | (byteValue >> lastOffset);
    }

    uint32_t Bitmap::size()
    {
        return bitmap_count;
    }
}