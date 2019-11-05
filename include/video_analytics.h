// Copyright (c) 2019 Intel Corporation.
//
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

/**
 * @file
 * @brief Base class VideoAnalytics
 */

#ifndef _EIS_VA_VIDEOANALYTICS_H
#define _EIS_VA_VIDEOANALYTICS_H

#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <memory>
#include <exception>
#include <vector>
#include <eis/utils/thread_safe_queue.h>
#include <eis/utils/logger.h>
#include <eis/udf/udf_manager.h>
#include <eis/utils/config.h>
#include <eis/utils/json_config.h>
#include <eis/utils/frame.h>
#include <eis/msgbus/msgbus.h>
#include <eis/config_manager/env_config.h>
#include <eis/config_manager/config_manager.h>

using namespace eis::utils;
using namespace eis::udf;
using namespace eis::config_manager;

namespace eis {
	namespace va {

        // VideoAnalytics class
        class VideoAnalytics {

            //App name value read from compose file
            std::string m_app_name;

            // Env config
            EnvConfig* m_env_config;

            // ConfigManager client
            config_mgr_t* m_config_mgr_client;

            // Classifier configuration object
            config_t* m_udf_cfg;

            // EIS UDFManager
            // UDFLoader::UdfManager* m_udf_manager;

            // UDF input queue
            FrameQueue* m_udf_input_queue;

            // UDF output queue
            FrameQueue* m_udf_output_queue;

            // EIS MsgBus Publisher
            msgbus::Publisher* m_publisher;

            // EIS MsgBus Subscriber
            msgbus::Subscriber<eis::utils::Frame>* m_subscriber;

            /**
             * Frees up the dynamically allotted memory
             */
            void cleanup();

        public:
            //Constructor
            VideoAnalytics();

            //Destructor
            ~ VideoAnalytics();

            // Has the actual flow as given in the sequence diagram
            // Start the VA pipeline in order of Publisher, UDFManager, Subscriber 
            void start();

            // stop the Video Analytics
            void stop();

        };
    };
};

#endif
