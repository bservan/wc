// wc.cpp : Defines the entry point for the application.
// written by bservan (Berhem Servan Gok)

#include "cmdline.h"

int main(int argc, char **argv)
{
    CmdLine cmdline(--argc, ++argv);
    cmdline.process();
    return 0;
}
