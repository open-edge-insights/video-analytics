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

#include "video_analytics.h"

#define MAX_CONFIG_KEY_LENGTH 40
#define DEFAULT_QUEUE_SIZE 10

using namespace eis::va;
using namespace eis::utils;
using namespace eis::msgbus;

VideoAnalytics::VideoAnalytics() {

    try {

    LOG_INFO_0("Fetching config");

    m_app_name = getenv("AppName");
    m_env_config = new EnvConfig();
    m_config_mgr_client = m_env_config->get_config_mgr_client();

    char config_key[MAX_CONFIG_KEY_LENGTH];
    sprintf(config_key, "/%s/config", m_app_name.c_str());
    const char* va_config = m_config_mgr_client->get_config(config_key);

    LOG_INFO("App config: %s", va_config);
    config_t* config = json_config_new_from_buffer(va_config);
    if(config == NULL) {
        const char* err = "Failed to initialize configuration object";
        LOG_ERROR("%s", err);
        throw(err);
    }

    config_value_t* analytics_value = config->get_config_value(config->cfg,
                                                              "analytics");
    if(analytics_value == NULL) {
        const char* err = "analytics key is missing";
        LOG_ERROR("%s", err);
        throw(err);
    } else {

        config_value_t* analytics_queue_cvt = config_value_object_get(analytics_value,
                                                                     "queue_size");
        size_t queue_size = DEFAULT_QUEUE_SIZE;
        if(analytics_queue_cvt == NULL) {
            LOG_INFO("\"queue_size\" key missing, so using default queue size: \
                     %ld", queue_size);
        } else {
            if(analytics_queue_cvt->type != CVT_INTEGER) {
                const char* err = "\"queue_size\" value has to be of integer type";
                LOG_ERROR("%s", err);
                throw(err);
            }
            queue_size = analytics_queue_cvt->body.integer;
        }
        m_udf_input_queue = new FrameQueue(queue_size);

        config_value_t* udf_value = config->get_config_value(config->cfg,
                                                             "udf");
        if(udf_value == NULL) {
            LOG_INFO("\"udf\" key doesn't exist, so udf output queue is same as udf input queue!!")
            m_udf_output_queue = m_udf_input_queue;
        } else {
            m_udf_output_queue = new FrameQueue(queue_size);
            config_value_object_t* udf_cvt = udf_value->body.object;
            m_udf_cfg = config_new(udf_cvt, free, get_config_value);
        }
    }

    }
    catch(const std::exception& e)
    {
        LOG_ERROR("%s issue: seen in initializing the Video Analytics components",e.what());
    }
}

void VideoAnalytics::start() {
    try {

    LOG_INFO_0("Get pub topics from environmental varibales of VA container");
    std::vector<std::string> topics = m_env_config->get_topics_from_env("pub"); 
    if(topics.size() != 1) {
        const char* err = "Only one topic is supported. Neither more nor less";
        LOG_ERROR("%s", err);
        throw(err);
    }
    std::string topic_type = "pub";
    config_t* msgbus_config = m_env_config->get_messagebus_config(topics[0],
                                                                  topic_type);
        if(msgbus_config == NULL) {
            const char* err = "Failed to get publisher message bus config";
            LOG_ERROR("%s", err);
            throw(err);
        }
    m_publisher = new Publisher(msgbus_config, topics[0], (InputMessageQueue*)m_udf_output_queue);

    LOG_INFO_0("Get sub topics from environmental varibales of VA container");
    std::vector<std::string> sub_topics = m_env_config->get_topics_from_env("sub"); 
    if(sub_topics.size() != 1) {
        const char* err = "Only one topic is supported. Neither more nor less";
        LOG_ERROR("%s", err);
        throw(err);
    }
    std::string topic_type_sub = "sub";
    config_t* msgbus_config_sub = m_env_config->get_messagebus_config(sub_topics[0],
                                                                      topic_type_sub);
    if(msgbus_config_sub == NULL) {
            const char* err = "Failed to get subscriber message bus config";
            LOG_ERROR("%s", err);
            throw(err);
    }

    Subscriber<eis::utils::Frame>* m_subscriber = new Subscriber<eis::utils::Frame>(msgbus_config_sub, sub_topics[0], (OutputMessageQueue*)m_udf_input_queue);

    m_publisher->start();
    LOG_INFO_0("Publisher thread started...");

    m_subscriber->start();
    LOG_INFO_0("Subscriber thread started...");

    #if 0
    LOG_INFO_0("Starting udf manager");
    m_udf_manager = new UdfManager(
        m_udf_cfg, m_udf_input_queue, m_udf_output_queue);
    m_udf_manager->start();
    LOG_INFO_0("Started udf manager");
    #endif

    while(1) {
        usleep(2);
    }

    }
    catch(const std::exception& e1) {
        LOG_ERROR("Exception occurred during Video_analytics PUBLISHER start");
    }

}

void VideoAnalytics::stop() {
    if(m_subscriber) {
        m_subscriber->stop();
        delete m_subscriber;
    }
    #if 0
    if(m_udf_manager) {
        m_udf_manager->stop();
        delete m_udf_manager;
    }
    #endif
    if(m_publisher) {
        m_publisher->stop();
        delete m_publisher;
    }
}

void VideoAnalytics::cleanup() {
    stop();
    if(m_udf_cfg) {
        delete m_udf_cfg;
    }
    if(m_config_mgr_client) {
        delete m_config_mgr_client;
    }
    if(m_env_config) {
        delete m_env_config;
    }
}

VideoAnalytics::~VideoAnalytics() {
    cleanup();
}