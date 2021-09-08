#include "stdafx.h"
#include "FindObjectsTask.h"
#include <ActivePrimeExecutionNote.h>

using namespace std;
using namespace Common;
using namespace ServerBackgroundThread;

namespace ObjectTree {

    void parseInput (
        const std::string& input,
        std::vector<std::pair<std::string, bool> >& names
    );

    void quetyObjectDescriptors (
        OciConnect& connect,
        std::vector<std::pair<std::string, bool> > names,
        std::vector<ObjectDescriptor>& result
    );

FindObjectsTask::FindObjectsTask (const std::string& input, void* owner)
: Task("Find Object(s)", owner),
  m_input(input)
{
    parseInput(input, m_names);
}

void FindObjectsTask::DoInBackground (OciConnect& connect)
{
    try
    {
        ActivePrimeExecutionOnOff onOff;

        if (m_names.size() > 0)
            quetyObjectDescriptors(connect, m_names, m_result);
    }
    catch (const FindObjectException& x)
    {
        m_error = x.what();
    }
}

}; // namespace ObjectTree
