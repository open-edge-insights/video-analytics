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
 * @brief VideoAnalytics Implementation
 */

#include "eis/va/video_analytics.h"

#define MAX_CONFIG_KEY_LENGTH 40
#define DEFAULT_QUEUE_SIZE 10

using namespace eis::va;
using namespace eis::utils;
using namespace eis::msgbus;

VideoAnalytics::VideoAnalytics(
        std::condition_variable& err_cv, EnvConfig* env_config) :
    m_err_cv(err_cv)
{
    LOG_DEBUG_0("Fetching config");

    m_app_name = getenv("AppName");
    config_mgr_t* config_mgr = env_config->get_config_mgr_client();

    // Get the configuration from the configuration manager
    char config_key[MAX_CONFIG_KEY_LENGTH];
    sprintf(config_key, "/%s/config", m_app_name.c_str());
    const char* va_config = config_mgr->get_config(config_key);

    LOG_DEBUG("App config: %s", va_config);

    // Parse the configuration
    config_t* config = json_config_new_from_buffer(va_config);
    if(config == NULL) {
        const char* err = "Failed to initialize configuration object";
        LOG_ERROR("%s", err);
        throw(err);
    }

    // Get queue size configuration
    config_value_t* queue_cvt = config->get_config_value(
            config->cfg, "queue_size");
    size_t queue_size = DEFAULT_QUEUE_SIZE;
    if(queue_cvt == NULL) {
        LOG_INFO("\"queue_size\" key missing, using default queue size: \
                %ld", queue_size);
    } else {
        if(queue_cvt->type != CVT_INTEGER) {
            const char* err = "\"queue_size\" value has to be of integer type";
            LOG_ERROR("%s", err);
            config_destroy(config);
            config_value_destroy(queue_cvt);
            throw(err);
        }

        queue_size = queue_cvt->body.integer;
    }

    // Get configuration values for the subscriber
    LOG_DEBUG_0("Parsing VA subscription topics");
    std::vector<std::string> sub_topics = env_config->get_topics_from_env("sub");
    if(sub_topics.size() != 1) {
        const char* err = "Only one topic is supported. Neither more nor less";
        LOG_ERROR("%s", err);
        config_destroy(config);
        throw(err);
    }

    std::string topic_type_sub = "sub";
    config_t* msgbus_config_sub = env_config->get_messagebus_config(
            sub_topics[0], topic_type_sub);
    if(msgbus_config_sub == NULL) {
        const char* err = "Failed to get subscriber message bus config";
        LOG_ERROR("%s", err);
        config_destroy(config);
        throw(err);
    }

    // Get configuration values for the publisher
    LOG_DEBUG_0("Parsing VA publisher topics");
    std::vector<std::string> topics = env_config->get_topics_from_env("pub");
    if(topics.size() != 1) {
        const char* err = "Only one topic is supported. Neither more nor less";
        LOG_ERROR("%s", err);
        config_destroy(config);
        throw(err);
    }

    std::string topic_type = "pub";
    config_t* pub_config = env_config->get_messagebus_config(
            topics[0], topic_type);
    if(pub_config == NULL) {
        const char* err = "Failed to get publisher message bus config";
        LOG_ERROR("%s", err);
        config_destroy(config);
        throw(err);
    }

    // Initialize input and output queues
    m_udf_input_queue = new FrameQueue(queue_size);
    m_udf_output_queue = new FrameQueue(queue_size);

    // Initialize Publisher
    m_publisher = new Publisher(
            pub_config, m_err_cv, topics[0],
            (MessageQueue*) m_udf_output_queue);

    // Initialize UDF Manager
    m_udf_manager = new UdfManager(
            config, m_udf_input_queue, m_udf_output_queue);

    // Initialize subscriber
    m_subscriber = new Subscriber<eis::udf::Frame>(
            msgbus_config_sub, m_err_cv, sub_topics[0],
            (MessageQueue*) m_udf_input_queue);
}

void VideoAnalytics::start() {
    m_publisher->start();
    LOG_INFO_0("Publisher thread started...");

    m_udf_manager->start();
    LOG_INFO_0("Started udf manager");

    m_subscriber->start();
    LOG_INFO_0("Subscriber thread started...");
}

void VideoAnalytics::stop() {
    m_subscriber->stop();
    m_udf_manager->stop();
    m_publisher->stop();
}

VideoAnalytics::~VideoAnalytics() {
    // NOTE: stop() methods are automatically called in all destructors
    delete m_subscriber;
    delete m_udf_manager;
    delete m_publisher;
}
