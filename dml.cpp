#include "dml.h"
#include "CmdExt.h"
#include "CmdList.h"

#include <fstream>

using namespace std;

DEFINE_CMD(dml)
{
    if (args.size() < 2)
        EXT_F_ERR("Usage: !dk load_dml <dml_filename>\n");

    show_dml(args[1]);
}

void show_dml(string dml_path)
{
    try
    {
        ifstream ifs(dml_path);

        if (ifs.good())
        {
            string content;

            while (getline(ifs, content))
            {
                content.push_back('\n');

                EXT_F_DML(content.c_str());
            }
        }
    }
    FC;
}