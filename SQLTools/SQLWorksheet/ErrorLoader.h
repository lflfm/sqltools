#pragma once

#include "DocumentProxy.h"

namespace ErrorLoader
{

    struct comp_error { 
        int line, position; 
        string text; 
        bool skip; 
        comp_error () : line(0), position(0), skip(false) {}
    };

    int Load (DocumentProxy& doc, OciConnect&, const char* owner, const char* name, const char* type, int line);

    void SubmitLoadTask (DocumentProxy& doc, const char* owner, const char* name, const char* type, int line);
};

