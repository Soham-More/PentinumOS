#pragma once

#include <includes.h>
#include "vector.hpp"

namespace std
{
    class string
    {
        private:
            char* buffer;
            size_t byte_size;

        public:
            struct view
            {
                const string& str;

                size_t start = 0;
                size_t end = invalid_u32;
                size_t step = 1;

                std::string as_string ();
                inline operator std::string() { return this->as_string(); }
            };

            static const size_t npos = -1;

            string();
            string(const string& str);
            string(string&& str);
            string(const char cstring[]);
            string(const u16 wstring[]);
            string(const char fill, size_t _len);
            string(const char* cstring, size_t _len);
            string(const u16* wstring, size_t _arraylen);

            // assumes ASCII charecters
            const char* c_str() const;
            // return internal buffer(and do not retain ownership)
            inline std::string copy() { return std::string(*this); }
            char* take();
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
            void ljust(const char fill, size_t target_len);

            // changes all charecter to uppercase
            void to_upper();
            // changes all charecter to lowercase
            void to_lower();

            // add charecters in given range
            //string substring(size_t start, size_t end = (size_t)-1, size_t step = 1) const;

            // get a substring
            view substring(size_t start, size_t end = (size_t)-1, size_t step = 1) const;

            friend string operator+(const string& lhs, const string& rhs);
            friend string operator+(const string& lhs, const char& rhs);
            friend string operator+=(string& lhs, const string& rhs);
            friend string operator+=(string& lhs, const char& rhs);
            friend bool operator==(const string& lhs, const string& rhs);

            friend bool operator==(const string::view& lhs, const string& rhs);
            friend bool operator==(const string& lhs, const string::view& rhs);
            friend bool operator==(const string::view& lhs, const string::view& rhs);
            
            char& operator[](size_t index) const;
            string operator=(const string& str);
            void operator=(string&& str);

            operator view();

            ~string();
    };

    string operator+(const string& lhs, const string& rhs);
    string operator+(const string& lhs, const char& rhs);
    string operator+=(string& lhs, const string& rhs);
    string operator+=(string& lhs, const char& rhs);
    bool operator==(const string& lhs, const string& rhs);

    bool operator==(const string::view& lhs, const string& rhs);
    bool operator==(const string& lhs, const string::view& rhs);
    bool operator==(const string::view& lhs, const string::view& rhs);

    bool operator!=(const string::view& lhs, const string& rhs);
    bool operator!=(const string& lhs, const string::view& rhs);
    bool operator!=(const string::view& lhs, const string::view& rhs);
}

