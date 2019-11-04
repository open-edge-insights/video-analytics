// Copyright (c) 2019 Intel Corporation.  //
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#include <iostream>
#include "video_analytics.h"
#include <string>

using namespace eis::va;
using namespace eis::utils;

void usage(const char* name) {
    printf("usage: %s [-h|--help]\n", name);
}

int main(int argc, char** argv)
{
    // Initialize log level default
    set_log_level(LOG_LVL_INFO);

    VideoAnalytics* va = NULL;
    try {
        if(argc >= 2) {
            LOG_ERROR("Usage: %s [-h|--help] \n",argv[0]);
        }
        va = new VideoAnalytics();
        va->start();
    }
    catch(const std::exception& e) {
        LOG_ERROR("Exception occurred: %s", e.what());
        va->stop();
        return -1;
    }
    return 0;
    
}



