#pragma once

#include "CmdInterface.h"

#include <sstream>

DECLARE_CMD(page)
DECLARE_CMD(pages)
DECLARE_CMD(hole)

#pragma pack(push, 1)
struct PTE64
{
    size_t Valid : 1;
    size_t Write : 1;
    size_t Owner : 1;
    size_t WriteThrough : 1;
    size_t CacheDisabled : 1;
    size_t Accessed : 1;
    size_t Dirty : 1;
    size_t LargePage : 1;
    size_t Global : 1;
    size_t SoftCopyOnWrite : 1;
    size_t SoftPrototype : 1;
    size_t SoftWrite : 1;
    size_t PageFrameNumber : 28;
    size_t Reserved : 12;
    size_t SoftWorkingSetIndex : 11;
    size_t NoExecute : 1;

    PTE64(size_t source)
    {
        memcpy(this, &source, 8);
    }

    size_t PFN()
    {
        return (size_t)PageFrameNumber << 12;
    }

    bool valid() { return Valid != 0; }
    bool write() { return Write != 0; }
    bool owner() { return Owner != 0; }
    bool writeThrough() { return WriteThrough != 0; }
    bool cache() { return CacheDisabled == 0; }
    bool accessed() { return Accessed != 0; }
    bool dirty() { return Dirty != 0; }
    bool large() { return LargePage != 0; }
    bool global() { return Global != 0; }
    bool softCow() { return SoftCopyOnWrite != 0; }
    bool softProto() { return SoftPrototype != 0; }
    bool softWrite() { return SoftWrite != 0; }
    bool nx() { return NoExecute != 0; }
    size_t workingsetIndex() { return SoftWorkingSetIndex; }

    string str()
    {
        stringstream ss;
        ss << (valid() ? "Valid" : "invalid") << " "
            << (write() ? "Write" : "readonly") << " "
            << (owner() ? "Usermode" : "kernelmode") << " "
            << (accessed() ? "Accessed" : "noaccess") << " "
            << (dirty() ? "Dirty" : "no-dirty") << " "
            << (large() ? "Large" : "no-large") << " "
            << (global() ? "Global" : "no-global") << " "
            << (softCow() ? "CopyOnWrite" : "no-copyonwrite") << " "
            << (softWrite() ? "SoftWrite" : "no-softwrite") << " "
            << (nx() ? "NX" : "no-NX") << " ";
        ;

        return ss.str();
    }
};
#pragma pack(pop)

void dump_page_info(size_t addr);
void dump_pages_around(size_t addr);
void dump_hole(size_t addr);
