#pragma once

#include "ObjectTree_Builder.h"
#include "ServerBackgroundThread\TaskQueue.h"

namespace ObjectTree {

    class FindObjectsTask : public ServerBackgroundThread::Task
    {
    public:
        FindObjectsTask  (const std::string& input, void* owner);

        void DoInBackground (OciConnect& connect);
        void ReturnInForeground () = 0;

    protected:
        std::vector<std::pair<std::string, bool> > m_names;
        std::vector<ObjectDescriptor> m_result;
        string m_input, m_error;
    };

}; // namespace ObjectTree
