#pragma once

#include <includes.h>
#include <std/Utils/vector.hpp>

namespace std
{
    class string
    {
        private:
            char* strBuffer;
            size_t byteSize;

        public:
            static const size_t npos = -1;

            string();
            string(const string& str);
            string(string&& str);
            string(const char cstring[]);
            string(const uint16_t cstring[]);
            string(const char fill, size_t _len);
            string(const char* cstring, size_t _len);
            string(const uint16_t* wstring, size_t _blen);

            const char* c_str() const;
            size_t size() const;
            bool empty() const;

            // finds the charecter, and returns first occurance after offset chars index
            size_t find(char c, size_t offset = 0) const;
            // counts the number of occurances of char c
            size_t count(char c) const;
            // splits string into substrings 
            void split(char splitChar, std::vector<string>& list) const;

            // add padding to right, to make the string len bytes in total
            // does nothing if len is smaller than size()
            void ljust(const char fill, size_t len);

            // changes all charecter to uppercase
            void to_upper();

            friend string operator+(const string& lhs, const string& rhs);
            friend string operator+(const string& lhs, const char& rhs);
            friend string operator+=(string& lhs, const string& rhs);
            friend string operator+=(string& lhs, const char& rhs);
            friend bool operator==(const string& lhs, const string& rhs);
            
            char& operator[](size_t index) const;
            string operator=(const string& str);
            void operator=(string&& str);

            ~string();
    };

    string operator+(const string& lhs, const string& rhs);
    string operator+(const string& lhs, const char& rhs);
    string operator+=(string& lhs, const string& rhs);
    string operator+=(string& lhs, const char& rhs);
    bool operator==(const string& lhs, const string& rhs);
}

