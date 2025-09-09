#pragma once

#include <includes.h>
#include <cpu/exceptions.hpp>
#include <std/errors.hpp>

#include <std/basic.hpp>
#include "../misc/init_list.hpp"
#include "../misc/references.hpp"
#include "../memory/heap.hpp"

namespace std
{
    template<class T>
    class vector
    {
        private:
            T* data;
            size_t data_size;
            size_t reserved_size;

            void destroy()
            {
                // if data is not null, then free it
                if(data == nullptr) return;
                // call destructor
                for(size_t i = 0; i < data_size; i++)
                {
                    data[i].~T();
                }
                std::free(data);
                data = nullptr;
                data_size = 0;
                reserved_size = 0;
            }
        public:
            using iterator = T*;
            using const_iterator = const T*;

            vector() : data(nullptr), data_size(0), reserved_size(0) { ; }

            vector(const std::initializer_list<T>& list)
            {
                if(list.size() == 0)
                {
                    data = nullptr;
                    data_size = 0;
                    reserved_size = 0;
                    return;
                }

                data = (T*)std::malloc(list.size() * sizeof(T));

                for(size_t i = 0; i < list.size(); i++)
                {
                    data[i] = *(list.begin() + i);
                }

                data_size = list.size();
                reserved_size = list.size();
            }
            vector(const std::vector<T>& list)
            {
                if(list.data_size == 0)
                {
                    data = nullptr;
                    data_size = 0;
                    reserved_size = 0;
                    return;
                }

                data = (T*)std::malloc(list.size() * sizeof(T));

                for(size_t i = 0; i < list.size(); i++)
                {
                    data[i] = list[i];
                }

                data_size = list.data_size;
                reserved_size = list.reserved_size;
            }
            vector(std::vector<T>&& list)
            {
                data = list.data; list.data = nullptr;
                data_size = list.data_size; list.data_size = 0;
                reserved_size = list.reserved_size; list.reserved_size = 0;
            }
            vector(T* _data, size_t _len)
            {
                if(_data == nullptr || _len == 0)
                {
                    data = nullptr;
                    data_size = 0;
                    reserved_size = 0;
                    return;
                }

                data = (T*)std::malloc(_len * sizeof(T));

                for(size_t i = 0; i < _len; i++)
                {
                    new (static_cast<void*>(&data[i])) T(_data[i]);
                }

                data_size = _len;
                reserved_size = _len;
            }

            void push_back(const T& item)
            {
                // if data is nullptr, allocate a page
                if(data == nullptr)
                {
                    data = reinterpret_cast<T*>(std::malloc(2 * sizeof(T)));
                    new (static_cast<void*>(&data[data_size])) T(item);
                    data_size = 1;
                    reserved_size = 2;

                    return;
                }

                // if size is less than reserved size, add element
                if(data_size < reserved_size)
                {
                    new (static_cast<void*>(&data[data_size])) T(item);
                    data_size++;
                }
                // else relloc memory
                else
                {
                    data = reinterpret_cast<T*>(realloc(data, 2 * reserved_size * sizeof(T)));
                    new (static_cast<void*>(&data[data_size])) T(item);
                    data_size++;
                    reserved_size = 2 * reserved_size;
                }
            }
            void push_back(T&& item)
            {
                // if data is nullptr, allocate
                if(data == nullptr)
                {
                    data = reinterpret_cast<T*>(std::malloc(2 * sizeof(T)));
                    //std::memset(&data[data_size], 0, sizeof(T));
                    new (static_cast<void*>(&data[data_size])) T(std::move(item));
                    data_size = 1;
                    reserved_size = 2;
                    return;
                }

                // if size is less than reserved_size then, add element
                if(data_size < reserved_size)
                {
                    new (static_cast<void*>(&data[data_size])) T(std::move(item));
                    data_size++;
                }
                // else relloc memory
                else
                {
                    data = reinterpret_cast<T*>(realloc(data, 2 * reserved_size * sizeof(T)));
                    new (static_cast<void*>(&data[data_size])) T(std::move(item));
                    data_size++;
                    reserved_size = 2 * reserved_size;
                }
            }

            // remove item at the end of list
            // then return the item(moved out!)
            T& pop_back()
            {
                if(data_size == 0) x86_raise(STD_OUT_OF_RANGE_INDEX);
                data_size--;
                return std::move(data[data_size]);
            }
            // get item at the end of the list
            T& back()
            {
                if(data_size == 0) x86_raise(STD_OUT_OF_RANGE_INDEX);
                return *(data + data_size - 1);
            }
            // get item at the end of the list
            const T& back() const
            {
                if(data_size == 0) x86_raise(STD_OUT_OF_RANGE_INDEX);
                return *(data + data_size - 1);
            }

            // basic iterators
            iterator begin() { return data; }
            iterator end() { return data + data_size; }
            const_iterator begin() const { return data; }
            const_iterator end() const { return data + data_size; }

            size_t size() const { return data_size; }
            bool empty() const { return data_size == 0; }
            T& operator[](u32 index)
            {
                if(index >= this->data_size) x86_raise(STD_OUT_OF_RANGE_INDEX);
                return data[index];
            }
            const T& operator[](u32 index) const
            {
                if(index >= this->data_size) x86_raise(STD_OUT_OF_RANGE_INDEX);
                return data[index];
            }

            vector& operator=(vector&& other)
            {
                // TODO: this can be more efficient
                this->destroy();
                data = other.data;
                data_size = other.data_size;
                reserved_size = other.reserved_size;
                return *this;
            }

            ~vector()
            {
                // if data is not null, then free it
                if(data != nullptr)
                {
                    // call destructor
                    for(size_t i = 0; i < data_size; i++)
                    {
                        data[i].~T();
                    }
                    free(data);
                    data = nullptr;
                    data_size = 0;
                    reserved_size = 0;
                }
            }
    };
}
