#include "String.hpp"

#include <std/stdmem.hpp>
#include <std/Utils/string.h>

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
        size_t get_wstring_size(const uint16_t* str)
        {
            size_t size = 0;
            while(str[size] != 0) size++;
            return size;
        }
    }

    string::string() : strBuffer(nullptr), byteSize(0) {}
    string::string(const string& str)
    {
        byteSize = str.byteSize;
        strBuffer = (char*)std::malloc(byteSize + 1);
        memcpy(str.strBuffer, strBuffer, byteSize + 1);
    }
    string::string(string&& str)
    {
        byteSize = str.byteSize;
        strBuffer = str.strBuffer;
        str.strBuffer = nullptr;
    }
    string::string(const char cstring[])
    {
        byteSize = get_cstring_size(cstring);
        strBuffer = (char*)std::malloc(byteSize + 1);
        memcpy(cstring, strBuffer, byteSize + 1);
    }
    string::string(const uint16_t cstring[])
    {
        byteSize = get_wstring_size(cstring) / 2;
        strBuffer = (char*)std::malloc(byteSize + 1);

        copyWideStringToString(&cstring, strBuffer, byteSize * 2);
    }
    string::string(const char fill, size_t _len)
    {
        byteSize = _len;
        strBuffer = (char*)std::malloc(byteSize + 1);
        memset(strBuffer, fill, byteSize);
        strBuffer[byteSize] = 0;
    }
    string::string(const char* cstring, size_t _len)
    {
        byteSize = _len;
        strBuffer = (char*)std::malloc(_len + 1);
        memcpy(cstring, strBuffer, _len + 1);
        strBuffer[_len] = 0;
    }
    string::string(const uint16_t* wstring, size_t _blen)
    {
        byteSize = sizeof(_blen) / 2;
        strBuffer = (char*)std::malloc(byteSize + 1);

        copyWideStringToString(wstring, strBuffer, _blen);
    }

    const char* string::c_str() const
    {
        return strBuffer;
    }
    size_t string::size() const
    {
        return byteSize;
    }
    bool string::empty() const
    {
        return byteSize == 0;
    }
    size_t string::find(char c, size_t offset) const
    {
        for(size_t i = offset; i < byteSize; i++)
        {
            if(strBuffer[i] == c) return i;
        }

        return string::npos;
    }
    size_t string::count(char c) const
    {
        size_t cnt = 0;
        for(size_t i = 0; i < byteSize; i++)
        {
            if(strBuffer[i] == c) cnt++;
        }

        return cnt;
    }

    void string::split(char splitChar, std::vector<string>& list) const
    {
        size_t currentSplitBegin = 0;
        size_t currentSplitLength = 0;

        for(size_t i = 0; i < byteSize; i++)
        {
            if(strBuffer[i] == splitChar)
            {
                list.push_back(string(strBuffer + currentSplitBegin, currentSplitLength));

                currentSplitBegin = i + 1;
                currentSplitLength = 0;
                continue;
            }

            currentSplitLength++;
        }

        if(currentSplitBegin != byteSize)
        {
            list.push_back(string(strBuffer + currentSplitBegin, currentSplitLength));
        }
    }

    char& string::operator[](size_t index) const
    {
        // TODO: Throw exception if index >= bsize
        return strBuffer[index];
    }
    string string::operator=(const string& str)
    {
        if(strBuffer) std::free(strBuffer);

        byteSize = str.byteSize;
        strBuffer = (char*)std::malloc(byteSize + 1);
        memcpy(str.strBuffer, strBuffer, byteSize + 1);

        return *this;
    }
    void string::operator=(string&& str)
    {
        byteSize = str.byteSize;
        strBuffer = str.strBuffer;
        str.strBuffer = nullptr;
    }

    void string::ljust(const char fill, size_t len)
    {
        if(len < byteSize) return;

        char* freePtr = strBuffer;
        size_t freeSize = byteSize;
        byteSize = len;
        strBuffer = (char*)std::malloc(byteSize + 1);
        memset(strBuffer, fill, byteSize);
        strBuffer[byteSize] = 0;

        memcpy(freePtr, strBuffer, freeSize);

        std::free(freePtr);
    }
    void string::to_upper()
    {
        for(size_t i = 0; i < byteSize; i++)
        {
            strBuffer[i] = toUpper(strBuffer[i]);
        }
    }

    string::~string()
    {
        if(strBuffer) std::free(strBuffer);
    }

    string operator+(const string& lhs, const string& rhs)
    {
        string tmp;
        tmp.byteSize = lhs.byteSize + rhs.byteSize;
        tmp.strBuffer = (char*)std::malloc(tmp.byteSize + 1);

        memcpy(lhs.strBuffer, tmp.strBuffer, lhs.byteSize);
        memcpy(rhs.strBuffer, tmp.strBuffer + lhs.byteSize, rhs.byteSize);
        tmp.strBuffer[tmp.byteSize] = 0;

        return tmp;
    }

    string operator+(const string& lhs, const char& rhs)
    {
        string tmp;
        tmp.byteSize = lhs.byteSize + 1;
        tmp.strBuffer = (char*)std::malloc(tmp.byteSize + 1);

        memcpy(lhs.strBuffer, tmp.strBuffer, lhs.byteSize);
        tmp.strBuffer[lhs.byteSize] = rhs;
        tmp.strBuffer[tmp.byteSize] = 0;

        return tmp;
    }

    string operator+=(string& lhs, const string& rhs)
    {
        char* tmp = lhs.strBuffer;
        lhs = lhs + rhs;
        if(tmp) std::free(tmp);
        return lhs;
    }
    string operator+=(string& lhs, const char& rhs)
    {
        char* tmp = lhs.strBuffer;
        lhs = lhs + rhs;
        if(tmp) std::free(tmp);
        return lhs;
    }

    bool operator==(const string& lhs, const string& rhs)
    {
        if(lhs.byteSize != rhs.byteSize) return false;

        for(size_t i = 0; i < lhs.byteSize; i++)
        {
            if(lhs.strBuffer[i] != rhs.strBuffer[i]) return false;
        }

        return true;
    }
}

