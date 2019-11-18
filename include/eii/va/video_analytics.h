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
 * @brief VideoAnalytics interface
 */

#ifndef _EIS_VA_VIDEOANALYTICS_H
#define _EIS_VA_VIDEOANALYTICS_H

#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <memory>
#include <exception>
#include <vector>
#include <condition_variable>
#include <eis/utils/thread_safe_queue.h>
#include <eis/utils/logger.h>
#include <eis/udf/udf_manager.h>
#include <eis/utils/config.h>
#include <eis/utils/json_config.h>
#include <eis/udf/frame.h>
#include <eis/msgbus/msgbus.h>
#include <eis/config_manager/env_config.h>
#include <eis/udf/udf_manager.h>
#include <eis/config_manager/config_manager.h>

using namespace eis::utils;
using namespace eis::udf;
using namespace eis::config_manager;

namespace eis {
	namespace va {

        // VideoAnalytics class
        class VideoAnalytics {

            // EIS UDFManager
            UdfManager* m_udf_manager;

            // UDF input queue
            FrameQueue* m_udf_input_queue;

            // UDF output queue
            FrameQueue* m_udf_output_queue;

            // Error condition variable
            std::condition_variable& m_err_cv;

            // EIS MsgBus Publisher
            msgbus::Publisher* m_publisher;

            // EIS MsgBus Subscriber
            msgbus::Subscriber<eis::udf::Frame>* m_subscriber;

            // Encoding details
            EncodeType m_enc_type;
            int m_enc_lvl;

        public:
            /**
             * Constructor
             *
             * \note The environmental configuration memory is not managed
             *      by this object. It is managed by the caller.
             *
             * @param err_cv     - Error condition variable
             * @param env_config - Environmental configuration
             * @param va_config  - VideoAnalytics config
             */
            VideoAnalytics(
                    std::condition_variable& err_cv, EnvConfig* env_config, char* va_config);

            //Destructor
            ~VideoAnalytics();

            // Has the actual flow as given in the sequence diagram
            // Start the VA pipeline in order of Publisher, UDFManager, Subscriber
            void start();

            // stop the Video Analytics
            void stop();

        };
    }
}

#endif
