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

    string::string() : cstr(nullptr), bsize(0) {}
    string::string(const string& str)
    {
        bsize = str.bsize;
        cstr = (char*)std::malloc(bsize + 1);
        memcpy(str.cstr, cstr, bsize + 1);
    }
    string::string(string&& str)
    {
        bsize = str.bsize;
        cstr = str.cstr;
        str.cstr = nullptr;
    }
    string::string(const char cstring[])
    {
        bsize = get_cstring_size(cstring);
        cstr = (char*)std::malloc(bsize + 1);
        memcpy(cstring, cstr, bsize + 1);
    }
    string::string(const uint16_t cstring[])
    {
        bsize = get_wstring_size(cstring) / 2;
        cstr = (char*)std::malloc(bsize + 1);

        copyWideStringToString(&cstring, cstr, bsize * 2);
    }
    string::string(const char fill, size_t _len)
    {
        bsize = _len;
        cstr = (char*)std::malloc(bsize + 1);
        memset(cstr, fill, bsize);
        cstr[bsize] = 0;
    }
    string::string(const char* cstring, size_t _len)
    {
        bsize = _len;
        cstr = (char*)std::malloc(_len + 1);
        memcpy(cstring, cstr, _len + 1);
        cstr[_len] = 0;
    }
    string::string(const uint16_t* wstring, size_t _blen)
    {
        bsize = sizeof(_blen) / 2;
        cstr = (char*)std::malloc(bsize + 1);

        copyWideStringToString(wstring, cstr, _blen);
    }

    const char* string::c_str() const
    {
        return cstr;
    }
    size_t string::size() const
    {
        return bsize;
    }
    bool string::empty() const
    {
        return bsize == 0;
    }
    size_t string::find(char c, size_t offset) const
    {
        for(size_t i = offset; i < bsize; i++)
        {
            if(cstr[i] == c) return i;
        }

        return string::npos;
    }
    size_t string::count(char c) const
    {
        size_t cnt = 0;
        for(size_t i = 0; i < bsize; i++)
        {
            if(cstr[i] == c) cnt++;
        }

        return cnt;
    }

    void string::split(char splitChar, std::vector<string>& list) const
    {
        size_t currentSplitBegin = 0;
        size_t currentSplitLength = 0;

        for(size_t i = 0; i < bsize; i++)
        {
            if(cstr[i] == splitChar)
            {
                list.push_back(string(cstr + currentSplitBegin, currentSplitLength));

                currentSplitBegin = i + 1;
                currentSplitLength = 0;
                continue;
            }

            currentSplitLength++;
        }

        if(currentSplitBegin != bsize)
        {
            list.push_back(string(cstr + currentSplitBegin, currentSplitLength));
        }
    }

    char& string::operator[](size_t index) const
    {
        // TODO: Throw exception if index >= bsize
        return cstr[index];
    }
    string string::operator=(const string& str)
    {
        if(cstr) std::free(cstr);

        bsize = str.bsize;
        cstr = (char*)std::malloc(bsize + 1);
        memcpy(str.cstr, cstr, bsize + 1);

        return *this;
    }
    void string::operator=(string&& str)
    {
        bsize = str.bsize;
        cstr = str.cstr;
        str.cstr = nullptr;
    }

    void string::ljust(const char fill, size_t len)
    {
        if(len < bsize) return;

        char* freePtr = cstr;
        size_t freeSize = bsize;
        bsize = len;
        cstr = (char*)std::malloc(bsize + 1);
        memset(cstr, fill, bsize);
        cstr[bsize] = 0;

        memcpy(freePtr, cstr, freeSize);

        std::free(freePtr);
    }
    void string::to_upper()
    {
        for(size_t i = 0; i < bsize; i++)
        {
            cstr[i] = toUpper(cstr[i]);
        }
    }

    string::~string()
    {
        if(cstr) std::free(cstr);
    }

    string operator+(const string& lhs, const string& rhs)
    {
        string tmp;
        tmp.bsize = lhs.bsize + rhs.bsize;
        tmp.cstr = (char*)std::malloc(tmp.bsize + 1);

        memcpy(lhs.cstr, tmp.cstr, lhs.bsize);
        memcpy(rhs.cstr, tmp.cstr + lhs.bsize, rhs.bsize);
        tmp.cstr[tmp.bsize] = 0;

        return tmp;
    }

    string operator+(const string& lhs, const char& rhs)
    {
        string tmp;
        tmp.bsize = lhs.bsize + 1;
        tmp.cstr = (char*)std::malloc(tmp.bsize + 1);

        memcpy(lhs.cstr, tmp.cstr, lhs.bsize);
        tmp.cstr[lhs.bsize] = rhs;
        tmp.cstr[tmp.bsize] = 0;

        return tmp;
    }

    string operator+=(string& lhs, const string& rhs)
    {
        char* tmp = lhs.cstr;
        lhs = lhs + rhs;
        if(tmp) std::free(tmp);
        return lhs;
    }
    string operator+=(string& lhs, const char& rhs)
    {
        char* tmp = lhs.cstr;
        lhs = lhs + rhs;
        if(tmp) std::free(tmp);
        return lhs;
    }
}

