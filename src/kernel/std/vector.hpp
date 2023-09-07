#pragma once

#include <includes.h>
#include "initialiserList.hpp"
#include "memory.h"
#include "math.h"

namespace std
{
    template<class T>
    class vector
    {
        private:
            T* data;
            size_t dataSize;
            size_t pageSize;
        public:
            vector() : data(nullptr), dataSize(0), pageSize(0) { ; }

            // TODO: throw exception if no mem left
            vector(const std::initializer_list<T>& list)
            {
                data = (T*)alloc_cp((const void*)list.begin(), list.size() * sizeof(T));
                dataSize = list.size();
                pageSize = DivRoundUp(dataSize, PAGE_SIZE);
            }

            void push_back(const T& item)
            {
                // if data is nullptr, allocate a page
                if(data == nullptr)
                {
                    data = reinterpret_cast<T*>(alloc_page());
                    data[dataSize] = item;
                    dataSize = 1;
                    pageSize = 1;

                    return;
                }

                // if size is less than pages allocated then, add element
                if(dataSize < pageSize * PAGE_SIZE)
                {
                    data[dataSize] = item;
                    dataSize++;
                }
                // else relloc memory
                else
                {
                    data = reinterpret_cast<T*>(realloc_pages(data, pageSize, pageSize + 1));
                    data[dataSize] = item;
                    dataSize++;
                    pageSize++;
                }
            }

            size_t size() { return dataSize; }

            // TODO: Throw exception if index is not in range
            T& operator[](uint32_t index)
            {
                return data[index];
            }

            ~vector()
            {
                // if data is not null, then free it
                if(data != nullptr)
                {
                    free_pages(data, pageSize);
                    dataSize = 0;
                    pageSize = 0;
                }
            }
    };
}
