#pragma once

#include <includes.h>

namespace std
{
    class Bitmap
    {
        private:
            uint32_t bitmap_size = 0;   // size in bytes
            uint32_t bitmap_count = 0;
            uint8_t* bitmap = nullptr;
        
        public:
            static const uint32_t npos = 0;

            Bitmap() = default;
            Bitmap(uint8_t* _bitmap, uint32_t bitcount, bool defaultValue = true);

            bool get(uint32_t bitIndex);
            void set(uint32_t bitIndex, bool value);

            void setBits(uint32_t first, uint32_t size, bool value);

            // finds bit whose value is false
            uint32_t find_false();

            // finds n consecutive bits which are false
            uint32_t find_false_bits(uint32_t n);

            uint32_t size();

            ~Bitmap() = default;
    };
}
