#include "blkio.hpp"

namespace blkio
{
    struct request_link
    {
        request_t req;
        request_link* next;
    };
    
    static request_link* head;
    static request_link* tail;
    
    void init()
    {
        head = nullptr;
        tail = nullptr;
    }

    void submit_request(request_t &request)
    {
        request_link* link = new request_link();;
        link->req = request;
        
        if(tail == nullptr)
        {
            head = link;
            tail = head;
            return;
        }

        tail->next = link;
        tail = tail->next;
    }

    bool poll()
    {
        return head != nullptr;
    }

    request_t pop_request()
    {
        request_link* first = head;
        if(first == nullptr) return request_t{.type = REQ_INVALID};
        head = head->next;
        return first->req;
    }
}
