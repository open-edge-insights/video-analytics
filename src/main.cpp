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
 * @brief VideoAnalytics main program
 */

#include <unistd.h>
#include <condition_variable>
#include "eis/va/video_analytics.h"
#include <mutex>

#define MAX_CONFIG_KEY_LENGTH 40

using namespace eis::va;
using namespace eis::utils;

static VideoAnalytics* g_va = NULL;
static EnvConfig* g_config = NULL;
static char* g_va_config = NULL;
static std::condition_variable g_err_cv;

void usage(const char* name) {
    printf("Usage: %s \n", name);
}

void signal_callback_handler(int signum){
    if (signum == SIGTERM){
        LOG_INFO("Received SIGTERM signal, terminating Video Analytics");
    }else if(signum == SIGABRT){
        LOG_INFO("Received SIGABRT signal, terminating Video Analytics");
    }else if(signum == SIGINT){
        LOG_INFO("Received Ctrl-C, terminating Video Analytics");
    }

    if(g_va) {
        delete g_va;
    }
    if(g_config) {
        delete g_config;
    }
    exit(0);
}

void va_initialize(char* va_config){
    if(g_va) {
        delete g_va;
    }
    if(!g_config) {
        g_config = new EnvConfig();
    }

    g_va = new VideoAnalytics(g_err_cv, g_config, va_config);
    g_va->start();
}

void on_change_config_callback(char* key, char* va_config){
    if(strcmp(g_va_config, va_config)){
        // TODO: Dynamic config needs to be enabled later once it works
        // with python `udfs`
        // va_initialize(va_config);
        _Exit(-1);
    }
}

int main(int argc, char** argv) {
    signal(SIGINT, signal_callback_handler);
    signal(SIGABRT, signal_callback_handler);
    signal(SIGTERM, signal_callback_handler);
    config_mgr_t* config_mgr = NULL;
    log_lvl_t log_level = LOG_LVL_ERROR; // default log level is `ERROR`
    try {
        if(argc >= 2) {
            usage(argv[0]);
            return -1;
        }
        char* str_log_level = NULL;
        g_config = new EnvConfig();
        log_lvl_t log_level = LOG_LVL_ERROR;

        str_log_level = getenv("C_LOG_LEVEL");
        if(str_log_level == NULL) {
            throw "\"C_LOG_LEVEL\" env not set";
        }

        if(strcmp(str_log_level, "DEBUG") == 0) {
            log_level = LOG_LVL_DEBUG;
        } else if(strcmp(str_log_level, "INFO") == 0) {
            log_level = LOG_LVL_INFO;
        } else if(strcmp(str_log_level, "WARN") == 0) {
            log_level = LOG_LVL_WARN;
        } else if(strcmp(str_log_level, "ERROR") == 0) {
            log_level = LOG_LVL_ERROR;
        }
        set_log_level(log_level);

        std::string m_app_name = getenv("AppName");
        config_mgr_t* config_mgr = g_config->get_config_mgr_client();

        // Get the configuration from the configuration manager
        char config_key[MAX_CONFIG_KEY_LENGTH];
        sprintf(config_key, "/%s/config", m_app_name.c_str());
        g_va_config = config_mgr->get_config(config_key);
        LOG_DEBUG("App config: %s", g_va_config);

        LOG_DEBUG("Registering watch on config key: %s", config_key);
        config_mgr->register_watch_key(config_key, on_change_config_callback);

        va_initialize(g_va_config);

        std::mutex mtx;
        std::unique_lock<std::mutex> lk(mtx);
        g_err_cv.wait(lk);

        delete g_va;
        delete g_config;
        return -1;
    }catch(const std::exception& e) {
        LOG_ERROR("Exception occurred: %s", e.what());
        if(g_va) {
            delete g_va;
        }
        if(g_config) {
            delete g_config;
        }
        config_mgr_config_destroy(config_mgr);
        return -1;
    }
}