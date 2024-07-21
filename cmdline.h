#ifndef WC_CMDLINE_H
#define WC_CMDLINE_H

class CmdLine
{
private:
    class Impl;
    Impl *m_impl;

public:
    CmdLine(int argumentCount, char **commandOptions);
    ~CmdLine();

    void process();
};

#endif // WC_CMDLINE_H
