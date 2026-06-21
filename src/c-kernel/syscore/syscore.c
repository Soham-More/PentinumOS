#include "syscore.h"

#include <panic/panic.h>
#include <utils/heap.h>

#define syscore_FUNC_ECHO 0x0
#define syscore_FUNC_ALLOC_PAGES 0x1

void syscore_thread_entry() {
    log_info("syscore thread started successfully\n");

    // setup some test rpc
    while(true) {
        thread_rpc_desc_t rpc = kmt_rpc_listen();
        log_info("received rpc from caller {u16} to callee {u16} with function {u32}\n", rpc.caller, rpc.callee, rpc.function);

        if(rpc.function == syscore_FUNC_ECHO) {
            log_info("executing ECHO function\n");

            if(rpc.request_size != rpc.response_size) {
                log_info("request size {usize} does not match response size {usize}, returning error code\n", rpc.request_size, rpc.response_size);
                panic_on_err(kmt_rpc_return(&rpc, EINVAL), "Failed to return rpc response");
                continue;
            }
            if(IS_ERR_PTR(rpc.request) || IS_ERR_PTR(rpc.response)) {
                log_info("request/response buffer is/are invalid, returning error code\n");
                panic_on_err(kmt_rpc_return(&rpc, EINVPTR), "Failed to return rpc response");
                continue;
            }

            memcpy(rpc.response, rpc.request, rpc.request_size);
            log_info("echoed {usize} bytes\n", rpc.request_size);

            panic_on_err(kmt_rpc_return(&rpc, ESUCCESS), "Failed to return rpc response");
        } else {
            log_info("unknown rpc function {u32}, returning error code\n", rpc.function);
            panic_on_err(kmt_rpc_return(&rpc, EINVAL), "Failed to return rpc response");
        }
    }
}

err_t syscore_echo_test(const char* test) {
    thread_uid_t syscore_uid = kmt_get_thread("syscore");
    usize len = strlen(test) + 1;
    void* response_data = malloc(kmt_get_heap(), len);
    char* request_data = (char*)malloc(kmt_get_heap(), len);
    memcpy(request_data, test, len);

    err_t rpc_err = EPENDING;
    err_t err = kmt_rpc_call(syscore_uid, syscore_FUNC_ECHO, request_data, len, response_data, len, &rpc_err);
    if(err != ESUCCESS) {
        goto end;
    }
    if(rpc_err != ESUCCESS) {
        err = rpc_err;
        goto end;
    }

    // check if the response data matches the request data
    if(!strcmp(test, response_data, len)) {
        err = EINVAL;
        goto end;
    }

    end:
    //log_info("received response from syscore thread: {s}\n", response_data);
    free(kmt_get_heap(), response_data);
    return err;
}
