#include "list.h"
#include "CmdExt.h"
#include "CmdList.h"

#include <iomanip>

DEFINE_CMD(link)
{
    size_t head = EXT_F_IntArg(args, 1, 0);
    dig_link(head);
}
DEFINE_CMD(flink)
{
    size_t head = EXT_F_IntArg(args, 1, 0);
    size_t entry = EXT_F_IntArg(args, 2, 0);
    dig_link(head, entry);
}

void dig_link(size_t head)
{
    try
    {
        size_t addr = head;
        if (EXT_F_ValidAddr(addr))
        {
            size_t head = addr;
            for (size_t i = 0; i < 0x400; i++)
            {
                addr = dig_link(addr, head, head, i * 0x400, 0x400);

                if (addr == 0)
                    return;
            }

            EXT_F_OUT("\t[[ Too long list, unable to dump all ]]\n");
        }
        else
        {
            EXT_F_OUT(" --> 0x%I64x\n", addr);
        }
    }
    FC;
}

void dig_link(size_t head, size_t entry)
{
    try
    {
        size_t addr = head;
        if (EXT_F_ValidAddr(addr))
        {
            for (size_t i = 0; i < 0x100; i++)
            {
                addr = dig_link(addr, head, entry, i * 0x1000, 0x1000);

                if (addr == 0)
                    return;
            }

            EXT_F_OUT("\t[[ Too long list, unable to dump all ]]\n");
        }
        else
        {
            EXT_F_OUT(" --> 0x%I64x\n", addr);
        }
    }
    FC;
}

size_t dig_link(size_t addr, size_t head, size_t entry, size_t index, size_t count)
{
    try
    {
        stringstream ss;
        for (size_t i = 0; i < count; i++)
        {
            if (!EXT_F_ValidAddr(addr))
            {
                ss << "\t[[ broken link at "
                    << HEX_ADDR(addr)
                    << " ]]\n";

                EXT_F_OUT(ss.str().c_str());
                return 0;
            }

            size_t flink = EXT_F_READ<size_t>(addr);
            size_t blink = EXT_F_READ<size_t>(addr + 8);

            ss << hex << showbase << setfill(' ') << setw(10) << i + index
                << " [ "
                << HEX_ADDR(flink)
                << " , "
                << HEX_ADDR(blink)
                << " ] \n";


            if (flink == entry/* || blink == head*/)
            {
                ss << "\t[ found entry "
                    << HEX_ADDR(addr)
                    << " ]\n";

                EXT_F_OUT(ss.str().c_str());
                return 0;
            }

            if (flink == head/* || blink == head*/)
            {
                ss << "\t[ finished "
                    << HEX_ADDR(addr)
                    << " ]\n";

                EXT_F_OUT(ss.str().c_str());
                return 0;
            }

            addr = flink;
        }

        ss << "\t[[ ... next " << count << " entries ... ]]\n";

        EXT_F_OUT(ss.str().c_str());

        return addr;
    }
    FC;

    return addr;
}