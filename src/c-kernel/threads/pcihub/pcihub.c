#include "pcihub.h"

#include "impl/pci.h"

void pcihub_thread_entry(){
    log_info("pcihub thread started successfully\n");

    // get some pages from the syscore page manager context
    panic_on_err(syscore_alloc_pages(4, 0x08000000), "Failed to allocate pages from syscore page manager context");

    // test read/write to the allocated pages
    u32* test_page = (u32*)0x08000000;
    for(u32 i = 0; i < 1024; i++) {
        test_page[i] = i;
    }
    for(u32 i = 0; i < 1024; i++) {
        if(test_page[i] != i) {
            panic(PANIC_ASSERTION_FAILED, "test failed: test_page[{u32}] = {u32}, expected {u32}", i, test_page[i], i);
        }
    }

    log_debug("rw test passed!\n");

    // scan the PCI bus
    pci_enumerate();

    pci_pretty_print();

    for(;;);
}
