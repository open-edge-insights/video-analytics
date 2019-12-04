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
#include <atomic>
#include <csignal>

#define MAX_CONFIG_KEY_LENGTH 40

using namespace eis::va;
using namespace eis::utils;

static VideoAnalytics* g_va = NULL;
static char* g_va_config = NULL;
static std::condition_variable g_err_cv;
static config_mgr_t* g_config_mgr = NULL;
static env_config_t* g_env_config_client = NULL;
static std::atomic<bool> g_cfg_change;

void get_config_mgr(){
    std::string pub_cert_file = "";
    std::string pri_key_file = "";
    std::string trust_file = "";
    std::string m_app_name = getenv("AppName");
    bool dev_mode;

    std::string dev_mode_str=getenv("DEV_MODE");
    if (dev_mode_str == "false") {
        dev_mode = false;
    } else if (dev_mode_str == "true") {
        dev_mode = true;
    }

    if(!dev_mode) {
        pub_cert_file = "/run/secrets/etcd_" + m_app_name + "_cert";
        pri_key_file = "/run/secrets/etcd_" + m_app_name + "_key";
        trust_file = "/run/secrets/ca_etcd";
    }

    g_config_mgr = config_mgr_new("etcd",
                                 (char*)pub_cert_file.c_str(),
                                 (char*) pri_key_file.c_str(),
                                 (char*) trust_file.c_str());

}

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

    exit(0);
}

void va_initialize(char* va_config){
    if(g_va) {
        delete g_va;
    }
    if(!g_config_mgr) {
        get_config_mgr();
    }
    g_env_config_client = env_config_new();
    g_va = new VideoAnalytics(g_err_cv, g_env_config_client, va_config, g_config_mgr);
    g_va->start();
}

void on_change_config_callback(char* key, char* va_config){
    if(strcmp(g_va_config, va_config)){
        if(g_va) {
            delete g_va;
            g_va = NULL;
        }
        delete g_va_config;
        g_va_config = va_config;
        g_cfg_change.store(true);
        g_err_cv.notify_one();
    }
}

void clean_up(){
    if(g_va) {
        delete g_va;
    }
    config_mgr_config_destroy(g_config_mgr);
    env_config_destroy(g_env_config_client);
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
        get_config_mgr();
        char* str_log_level = NULL;
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


        // Get the configuration from the configuration manager
        char config_key[MAX_CONFIG_KEY_LENGTH];
        sprintf(config_key, "/%s/config", m_app_name.c_str());
        g_va_config = g_config_mgr->get_config(config_key);
        LOG_DEBUG("App config: %s", g_va_config);

        LOG_DEBUG("Registering watch on config key: %s", config_key);
        g_config_mgr->register_watch_key(config_key, on_change_config_callback);
        va_initialize(g_va_config);

        std::mutex mtx;

        while(true) {
            std::unique_lock<std::mutex> lk(mtx);
            g_err_cv.wait(lk);
            if(g_cfg_change.load()) {
                va_initialize(g_va_config);
                g_cfg_change.store(false);
            } else {
                break;
            }
        }

        clean_up();
    }catch(const std::exception& e) {
        LOG_ERROR("Exception occurred: %s", e.what());
        clean_up();
    }
    return -1;
}
