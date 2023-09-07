#pragma once

#include <includes.h>

namespace std
{
    class Bitmap
    {
        private:
            size_t bitmap_size = 0;   // size in bytes
            size_t bitmap_count = 0;
            uint8_t* bitmap = nullptr;
        
        public:
            static const size_t npos = 0;

            Bitmap() = default;
            Bitmap(uint8_t* _bitmap, size_t bitcount, bool defaultValue = true);

            bool get(size_t bitIndex);
            void set(size_t bitIndex, bool value);

            void setBits(size_t first, size_t size, bool value);

            // finds bit whose value is false
            size_t find_false();

            // finds n consecutive bits which are false
            size_t find_false_bits(uint32_t n);

            size_t size();

            ~Bitmap() = default;
    };
}
