#pragma once

#include <includes.h>
#include <std/Bitmap/initialiserList.hpp>
#include <std/stdmem.hpp>
#include <std/Utils/math.h>
#include <std/Generics/references.hpp>

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
            vector(const std::vector<T>& list)
            {
                data = (T*)alloc_cp((const void*)list.data, list.dataSize * sizeof(T));
                dataSize = list.dataSize;
                pageSize = list.pageSize;
            }
            vector(std::vector<T>&& list)
            {
                data = list.data; list.data = nullptr;
                dataSize = list.dataSize;
                pageSize = list.pageSize;
            }

            void push_back(const T& item)
            {
                // if data is nullptr, allocate a page
                if(data == nullptr)
                {
                    data = reinterpret_cast<T*>(alloc_page());
                    data[dataSize] = static_cast<T&&>(T()); // construct the object
                    data[dataSize] = item;
                    dataSize = 1;
                    pageSize = 1;

                    return;
                }

                // if size is less than pages allocated then, add element
                if(dataSize < pageSize * PAGE_SIZE)
                {
                    data[dataSize] = static_cast<T&&>(T()); // construct the object
                    data[dataSize] = item;
                    dataSize++;
                }
                // else relloc memory
                else
                {
                    data = reinterpret_cast<T*>(realloc_pages(data, pageSize, pageSize + 1));
                    data[dataSize] = static_cast<T&&>(T()); // construct the object
                    data[dataSize] = item;
                    dataSize++;
                    pageSize++;
                }
            }
            void push_back(T&& item)
            {
                // if data is nullptr, allocate a page
                if(data == nullptr)
                {
                    data = reinterpret_cast<T*>(alloc_page());
                    data[dataSize] = std::move(item);
                    dataSize = 1;
                    pageSize = 1;

                    return;
                }

                // if size is less than pages allocated then, add element
                if(dataSize < pageSize * PAGE_SIZE)
                {
                    data[dataSize] = std::move(item);
                    dataSize++;
                }
                // else relloc memory
                else
                {
                    data = reinterpret_cast<T*>(realloc_pages(data, pageSize, pageSize + 1));
                    data[dataSize] = std::move(item);
                    dataSize++;
                    pageSize++;
                }
            }

            // remove item at the end of list
            // then return a copy of it
            T pop_back()
            {
                dataSize--;
                return data[dataSize];
            }

            // get item at the end of the list
            T& back()
            {
                return data[dataSize - 1];
            }

            // get size of vector
            size_t size() const { return dataSize; }
            // get if the vector is empty
            bool empty() const { return dataSize == 0; }

            // TODO: Throw exception if index is not in range
            T& operator[](uint32_t index) const
            {
                return data[index];
            }

            ~vector()
            {
                // if data is not null, then free it
                if(data != nullptr)
                {
                    // call destructor
                    for(size_t i = 0; i < dataSize; i++)
                    {
                        data[i].~T();
                    }
                    free_pages(data, pageSize);
                    dataSize = 0;
                    pageSize = 0;
                }
            }
    };
}
