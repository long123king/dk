#include "list.h"
#include "CmdExt.h"
#include "CmdList.h"

#include <iomanip>

void dig_link(size_t head, size_t stop_at_entry)
{
    size_t current = head;
    size_t count = 0;
    const size_t MAX_NODES = 10000;

    EXT_F_OUT("Dumping list starting at: 0x%I64x", head);
    if (stop_at_entry != 0) {
        EXT_F_OUT(" (Stopping at: 0x%I64x)", stop_at_entry);
    }
    EXT_F_OUT("\n\n");

    while (count < MAX_NODES)
    {
        if (!EXT_F_ValidAddr(current))
        {
            std::stringstream ss;
            ss << "[Node " << count << "]\n"
               << "  Address : " << HEX_ADDR(current) << "\n"
               << "  Status  : [INVALID CURRENT ADDRESS]\n"
               << "\n";
            EXT_F_OUT("%s", ss.str().c_str());
            break;
        }

        size_t flink = 0;
        size_t blink = 0;
        
        try {
            flink = EXT_F_READ<size_t>(current);
            blink = EXT_F_READ<size_t>(current + 8);
        }
        catch (...) {
            std::stringstream ss;
            ss << "[Node " << count << "]\n"
               << "  Address : " << HEX_ADDR(current) << "\n"
               << "  Status  : [READ FAILED]\n"
               << "\n";
            EXT_F_OUT("%s", ss.str().c_str());
            break;
        }

        std::string status = "";
        bool stop = false;

        bool flink_valid = EXT_F_ValidAddr(flink);

        // Check 1: Flink validity
        if (!flink_valid) {
            if (flink == 0) {
                status += "[NULL FLINK (END)] "; 
                stop = true;
            }
            else {
                status += "[INVALID FLINK] ";
                stop = true;
            }
        }
        else {
            // Check 2: Backlink consistency (Flink->Blink == Current)
            size_t flink_blink = 0;
            try {
                flink_blink = EXT_F_READ<size_t>(flink + 8);
                if (flink_blink != current) {
                    status += "[CORRUPT: Flink->Blink != Curr] ";
                }
            }
            catch (...) {
                status += "[Flink->Blink Unreadable] ";
            }
        }

        // Check 3: Loop to head
        if (flink == head) {
            status += "[List Head] ";
            stop = true;
        }

        // Check 4: Target found
        if (stop_at_entry != 0 && current == stop_at_entry) {
            status += "[Found Target] ";
            stop = true;
        }

        std::stringstream ss;
        ss << "[Node " << count << "]\n"
           << "  Address : " << HEX_ADDR(current) << "\n"
           << "  Flink   : " << HEX_ADDR(flink) << "\n"
           << "  Blink   : " << HEX_ADDR(blink) << "\n";
        
        if (!status.empty()) {
            ss << "  Status  : " << status << "\n";
        }
        ss << "\n"; // Isolation empty line

        EXT_F_OUT("%s", ss.str().c_str());

        if (stop) break;

        current = flink;
        count++;
    }

    if (count == MAX_NODES) {
        EXT_F_OUT("... Stopped after %d nodes (Max limit reached) ...\n", MAX_NODES);
    }
}

DEFINE_CMD(link)
{
    if (args.size() < 2) {
        CMD_LIST->PrintUsage("link");
        return;
    }
    size_t head = EXT_F_IntArg(args, 1, 0);
    dig_link(head);
}

DEFINE_CMD(flink)
{
    if (args.size() < 2) {
        CMD_LIST->PrintUsage("flink");
        return;
    }
    size_t head = EXT_F_IntArg(args, 1, 0);
    size_t entry = 0;
    if (args.size() > 2) {
        entry = EXT_F_IntArg(args, 2, 0);
    }
    dig_link(head, entry);
}