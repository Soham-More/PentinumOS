#include "string.hpp"

#include <std/basic.hpp>
#include "../memory/heap.hpp"

namespace std
{
    namespace 
    {
        size_t get_cstring_size(const char* str)
        {
            size_t size = 0;
            while(str[size] != 0) size++;
            return size;
        }
        size_t get_wstring_size(const u16* str)
        {
            size_t size = 0;
            while(str[size] != 0) size++;
            return size;
        }

        // assumes src to be at most dstArrayLength - 1 big
        void copyWideStringToString(const u16 src[], char dst[], size_t dstArrayLength)
        {
            for(int i = 0; i < dstArrayLength - 1; i++)
            {
                dst[i] = src[i];
            }
            dst[dstArrayLength - 1] = 0;
        }
        bool strcmp(const char* a, const char* b, u32 length)
        {
            for(u32 i = 0; i < length; i++)
            {
                if(a[i] != b[i])
                {
                    return false;
                }
            }

            return true;
        }
        char toUpper(char c)
        {
            if(c >= 'a' && c <= 'z')
            {
                return c + 'A' - 'a';
            }

            return c;
        }
        char toLower(char c)
        {
            if(c >= 'A' && c <= 'Z')
            {
                return c - 'A' + 'a';
            }

            return c;
        }
    }

    string string::view::as_string()
    {
        string result;

        result.buffer = (char*)std::malloc((end - start)/step + 1);
        result.byte_size = (end - start)/step;

        for(u32 i = start, j = 0; i < end; i += step, j++)
        {
            result.buffer[j] = str.buffer[i];
        }
        // null terminate
        result.buffer[result.byte_size] = 0;

        return result;
    }

    string::string() : buffer(nullptr), byte_size(0) {}
    string::string(const string& str) : buffer(nullptr), byte_size(0)
    {
        if(str.buffer == nullptr) return;
        byte_size = str.byte_size;
        buffer = (char*)std::malloc(byte_size + 1);
        memcpy(str.buffer, buffer, byte_size + 1);
    }
    string::string(string&& str) : buffer(nullptr), byte_size(0)
    {
        if(str.buffer == nullptr) return;
        byte_size = str.byte_size;
        buffer = str.buffer;
        str.buffer = nullptr;
    }
    string::string(const char cstring[]) : buffer(nullptr), byte_size(0)
    {
        if(cstring == nullptr) return;
        byte_size = get_cstring_size(cstring);
        buffer = (char*)std::malloc(byte_size + 1);
        memcpy(cstring, buffer, byte_size + 1);
    }
    string::string(const u16 wstring[]) : buffer(nullptr), byte_size(0)
    {
        if(wstring == nullptr) return;
        byte_size = get_wstring_size(wstring);
        buffer = (char*)std::malloc(byte_size + 1);

        copyWideStringToString(wstring, buffer, byte_size + 1);
    }
    string::string(const char fill, size_t _len) : buffer(nullptr), byte_size(0)
    {
        byte_size = _len;
        buffer = (char*)std::malloc(byte_size + 1);
        memset(buffer, fill, byte_size);
        buffer[byte_size] = 0;
    }
    string::string(const char* cstring, size_t _len) : buffer(nullptr), byte_size(0)
    {
        if(cstring == nullptr) return;
        byte_size = _len;
        buffer = (char*)std::malloc(_len + 1);
        memcpy(cstring, buffer, _len + 1);
        buffer[_len] = 0;
    }
    string::string(const u16* wstring, size_t _arraylen) : buffer(nullptr), byte_size(0)
    {
        if(wstring == nullptr) return;
        byte_size = _arraylen;
        buffer = (char*)std::malloc(byte_size + 1);

        copyWideStringToString(wstring, buffer, byte_size + 1);
    }

    const char* string::c_str() const
    {
        return buffer;
    }
    char* string::take()
    {
        char* _buffer = buffer;
        buffer = nullptr;
        byte_size = 0;
        return _buffer;
    }
    size_t string::size() const
    {
        return byte_size;
    }
    bool string::empty() const
    {
        return byte_size == 0;
    }
    size_t string::find(char c, size_t offset) const
    {
        for(size_t i = offset; i < byte_size; i++)
        {
            if(buffer[i] == c) return i;
        }

        return string::npos;
    }
    size_t string::count(char c) const
    {
        size_t cnt = 0;
        for(size_t i = 0; i < byte_size; i++)
        {
            if(buffer[i] == c) cnt++;
        }

        return cnt;
    }

    void string::split(char splitChar, std::vector<string>& list) const
    {
        size_t currentSplitBegin = 0;
        size_t currentSplitLength = 0;

        for(size_t i = 0; i < byte_size; i++)
        {
            if(buffer[i] == splitChar)
            {
                list.push_back(string(buffer + currentSplitBegin, currentSplitLength));

                currentSplitBegin = i + 1;
                currentSplitLength = 0;
                continue;
            }

            currentSplitLength++;
        }

        if(currentSplitBegin != byte_size)
        {
            list.push_back(string(buffer + currentSplitBegin, currentSplitLength));
        }
    }

    char& string::operator[](size_t index) const
    {
        // TODO: Throw exception if index >= bsize
        return buffer[index];
    }
    string string::operator=(const string& str)
    {
        // TODO: change this code
        // some other code that uses malloc instead of new might
        // break
        if(buffer) std::free(buffer);

        byte_size = str.byte_size;
        buffer = (char*)std::malloc(byte_size + 1);
        memcpy(str.buffer, buffer, byte_size + 1);

        return *this;
    }
    void string::operator=(string&& str)
    {
        // if buffered free it
        if(buffer) std::free(buffer);
        byte_size = str.byte_size;
        buffer = str.buffer;
        str.buffer = nullptr;
    }

    void string::ljust(const char fill, size_t target_len)
    {
        if(target_len < byte_size) return;

        size_t prev_size = byte_size;

        byte_size = target_len;
        buffer = (char*)std::realloc(buffer, byte_size + 1);
        buffer[byte_size] = 0;
        
        // copy from the end
        for(size_t i = 0; i < prev_size; i++)
        {
            buffer[byte_size - i - 1] = buffer[prev_size - i - 1];
        }
        // fill the buffer properly
        memset(buffer, fill, byte_size - prev_size);
    }
    void string::to_upper()
    {
        for(size_t i = 0; i < byte_size; i++)
        {
            buffer[i] = toUpper(buffer[i]);
        }
    }
    void string::to_lower()
    {
        for(size_t i = 0; i < byte_size; i++)
        {
            buffer[i] = toLower(buffer[i]);
        }
    }
    string::view string::substring(size_t start, size_t end, size_t step) const
    {
        if(end > byte_size) end = byte_size;
        if(start >= byte_size) return string::view{.str=*this, .start=0, .end=0, .step=invalid_u32};        

        return string::view{.str=*this, .start=start, .end=end, .step=step};
    }

    string::operator view()
    {
        return this->substring(0);
    }

    string::~string()
    {
        if(buffer) std::free(buffer);
    }

    string operator+(const string& lhs, const string& rhs)
    {
        string tmp;
        tmp.byte_size = lhs.byte_size + rhs.byte_size;
        tmp.buffer = (char*)std::malloc(tmp.byte_size + 1);

        memcpy(lhs.buffer, tmp.buffer, lhs.byte_size);
        memcpy(rhs.buffer, tmp.buffer + lhs.byte_size, rhs.byte_size);
        tmp.buffer[tmp.byte_size] = 0;

        return tmp;
    }
    string operator+(const string& lhs, const char& rhs)
    {
        string tmp;
        tmp.byte_size = lhs.byte_size + 1;
        tmp.buffer = (char*)std::malloc(tmp.byte_size + 1);

        memcpy(lhs.buffer, tmp.buffer, lhs.byte_size);
        tmp.buffer[lhs.byte_size] = rhs;
        tmp.buffer[tmp.byte_size] = 0;

        return tmp;
    }

    string operator+=(string& lhs, const string& rhs)
    {
        lhs.buffer = (char*)std::realloc(lhs.buffer, lhs.byte_size + rhs.byte_size + 1);
        
        std::memcpy(rhs.buffer, lhs.buffer + lhs.byte_size, rhs.byte_size);
        lhs.buffer[lhs.byte_size + rhs.byte_size] = 0;

        lhs.byte_size += rhs.byte_size;
        return lhs;
    }
    string operator+=(string& lhs, const char& rhs)
    {
        lhs.buffer = (char*)std::realloc(lhs.buffer, lhs.byte_size + 2);
        lhs.buffer[lhs.byte_size] = rhs;
        lhs.buffer[lhs.byte_size + 1] = 0;
        lhs.byte_size++;
        return lhs;
    }

    bool operator==(const string& lhs, const string& rhs)
    {
        if(lhs.byte_size != rhs.byte_size) return false;

        for(size_t i = 0; i < lhs.byte_size; i++)
        {
            if(lhs.buffer[i] != rhs.buffer[i]) return false;
        }

        return true;
    }

    bool operator==(const string::view &lhs, const string &rhs)
    {
        size_t i = lhs.start, j = 0;

        for(; (i < lhs.end) && (j < rhs.byte_size); i += lhs.step, j ++)
        {
            if(lhs.str.buffer[i] != rhs.buffer[j]) return false;
        }

        return (i >= lhs.end) && (j >= rhs.byte_size);
    }

    bool operator==(const string &lhs, const string::view &rhs)
    {
        // TODO: optimize this
        size_t i = 0, j = rhs.start;

        for(; (i < lhs.byte_size) && (j < rhs.end); i += 1, j += rhs.step)
        {
            if(lhs.buffer[i] != rhs.str.buffer[j]) return false;
        }

        return (i >= lhs.byte_size) && (j >= rhs.end);
    }

    bool operator==(const string::view &lhs, const string::view &rhs)
    {
        // TODO: optimize this
        size_t i = lhs.start, j = rhs.start;

        for(; (i < lhs.end) && (j < rhs.end); i += lhs.step, j += rhs.step)
        {
            if(lhs.str.buffer[i] != rhs.str.buffer[j]) return false;
        }

        return (i >= lhs.end) && (j >= rhs.end);
    }
    bool operator!=(const string::view &lhs, const string &rhs)
    {
        return !(lhs == rhs);
    }
    bool operator!=(const string &lhs, const string::view &rhs)
    {
        return !(lhs == rhs);
    }
    bool operator!=(const string::view &lhs, const string::view &rhs)
    {
        return !(lhs == rhs);
    }
}
