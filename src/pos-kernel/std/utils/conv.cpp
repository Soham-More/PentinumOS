#include "conv.hpp"

namespace std
{
    std::string utos(u64 value)
    {
        char buffer[21];

        u64 built_val = 0;
        char* ptr = buffer;
        do
        {
            u64 c = value % 10;
            
            *ptr = ('0' + c);
            ptr++;
            
            built_val *= 10;
            built_val += c;
        } while(built_val != value);

        return std::string(buffer, ptr - buffer);
    }
}

