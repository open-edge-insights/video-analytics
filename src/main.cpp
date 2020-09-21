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
#include <stdbool.h>
#include <atomic>
#include <csignal>
#include <fstream>
#include <iostream>
#include <safe_lib.h>
#include <eis/utils/json_validator.h>
#include "eis/va/video_analytics.h"
#define MAX_CONFIG_KEY_LENGTH 250

using eis::va::VideoAnalytics;

static VideoAnalytics* g_va = NULL;
static char* g_va_config = NULL;
static std::condition_variable g_err_cv;
static config_mgr_t* g_config_mgr = NULL;
static env_config_t* g_env_config_client = NULL;
static std::atomic<bool> g_cfg_change;

void get_config_mgr(char* str_app_name) {
    char pub_cert_file[MAX_CONFIG_KEY_LENGTH] = "";
    char pri_key_file[MAX_CONFIG_KEY_LENGTH] = "";
    char trust_file[MAX_CONFIG_KEY_LENGTH] = "";
    char storage_type[MAX_CONFIG_KEY_LENGTH];
    int ret = 0;
    std::string dev_mode_str = "";

    char* str_dev_mode = NULL;
    str_dev_mode = getenv("DEV_MODE");
    if (str_dev_mode == NULL) {
        throw "\"DEV_MODE\" env not set";
    } else {
        dev_mode_str = str_dev_mode;
    }

    bool dev_mode = false;
    if (dev_mode_str == "true") {
        dev_mode = true;
    }

    if (!dev_mode) {
        ret = snprintf(pub_cert_file, MAX_CONFIG_KEY_LENGTH,
                 "/run/secrets/etcd_%s_cert", str_app_name);
        if (ret < 0) {
            throw "failed to create pub_cert_file";
        }
        ret = snprintf(pri_key_file, MAX_CONFIG_KEY_LENGTH,
                 "/run/secrets/etcd_%s_key", str_app_name);
        if (ret < 0) {
            throw "failed to create pri_key_file";
        }
        ret = strncpy_s(trust_file, MAX_CONFIG_KEY_LENGTH + 1,
                  "/run/secrets/ca_etcd", MAX_CONFIG_KEY_LENGTH);
        if (ret != 0) {
            throw "failed to create trust file";
        }

        char* confimgr_cert = getenv("CONFIGMGR_CERT");
        char* confimgr_key = getenv("CONFIGMGR_KEY");
        char* confimgr_cacert = getenv("CONFIGMGR_CACERT");
        if (confimgr_cert && confimgr_key && confimgr_cacert) {
           ret = strncpy_s(pub_cert_file, MAX_CONFIG_KEY_LENGTH + 1,
                           confimgr_cert, MAX_CONFIG_KEY_LENGTH);
            if (ret != 0) {
                throw "failed to add cert to trust file";
            }
            ret = strncpy_s(pri_key_file, MAX_CONFIG_KEY_LENGTH + 1,
                            confimgr_key, MAX_CONFIG_KEY_LENGTH);
            if (ret !=0) {
                throw "failed to add key to trust file";
            }
            ret = strncpy_s(trust_file, MAX_CONFIG_KEY_LENGTH + 1,
                            confimgr_cacert, MAX_CONFIG_KEY_LENGTH);
            if (ret != 0 ){
                 throw "failed to add cacert to trust file";
            }

        }
    }

    ret = strncpy_s(storage_type, (MAX_CONFIG_KEY_LENGTH + 1),
                    "etcd", MAX_CONFIG_KEY_LENGTH);
    if (ret != 0){
        throw "failed to add storage type";
    }

    g_config_mgr = config_mgr_new(storage_type,
                                 pub_cert_file,
                                 pri_key_file,
                                 trust_file);

}

void usage(const char* name) {
    printf("Usage: %s \n", name);
}

void signal_callback_handler(int signum) {
    if (signum == SIGTERM) {
        LOG_INFO("Received SIGTERM signal, terminating Video Analytics");
    } else if (signum == SIGABRT) {
        LOG_INFO("Received SIGABRT signal, terminating Video Analytics");
    } else if (signum == SIGINT) {
        LOG_INFO("Received Ctrl-C, terminating Video Analytics");
    }

    if (g_va) {
        delete g_va;
    }

    exit(0);
}

void va_initialize(char* va_config, char* str_app_name) {
    if (g_va) {
        delete g_va;
    }
    if (!g_config_mgr) {
        get_config_mgr(str_app_name);
        if (!g_config_mgr) {
            const char* err = "config-mgr object creation failed.";
            throw(err);
        }
    }

    std::string app_name = "";
    app_name = str_app_name;

    g_env_config_client = env_config_new();
    g_va = new VideoAnalytics(g_err_cv, g_env_config_client, va_config,
                              g_config_mgr, app_name);
    g_va->start();
}

void on_change_config_callback(char* key, char* va_config) {
    if (strcmp(g_va_config, va_config)) {
        if (g_va) {
            delete g_va;
            g_va = NULL;
        }
        delete g_va_config;
        _Exit(-1);
        // TODO(nagdeep.gk@intel.com): Uncomment the below logic once the
        // dynamic cfg fix works as
        // expected
        // g_va_config = va_config;
        // g_cfg_change.store(true);
        // g_err_cv.notify_one();
    }
}

void clean_up() {
    if (g_va) {
        delete g_va;
    }
    config_mgr_config_destroy(g_config_mgr);
    env_config_destroy(g_env_config_client);
}

bool validate_config(char config_key[]) {
    // Writing to external file
    std::ofstream out;
    out.open("./config.json", std::ios::binary);
    out << config_key;
    out.close();

    WJReader readjson = NULL;
    WJReader readschema = NULL;
    WJElement json = NULL;
    WJElement schema = NULL;

    // Fetch config file
    FILE* config_fp = fopen("./config.json", "r");
    if(config_fp == NULL) {
        return false;
    }
    readjson = WJROpenFILEDocument(config_fp, NULL, 0);
    if(readjson == NULL) {
        LOG_ERROR_0("config json could not be read");
        if(config_fp != NULL) {
            fclose(config_fp);
        }
        return false;
    }
    json = WJEOpenDocument(readjson, NULL, NULL, NULL);
    if(json == NULL) {
        LOG_ERROR_0("config json could not be read");
        if(readjson != NULL) {
            free(readjson);
        }
        if(config_fp != NULL) {
            fclose(config_fp);
        }
        return false;
    }

    // Fetch schema file
    FILE* schema_fp = fopen("./VideoAnalytics/schema.json", "r");
    if(schema_fp == NULL) {
        if(config_fp != NULL) {
            fclose(config_fp);
        }
        return false;
    }
    readschema = WJROpenFILEDocument(schema_fp, NULL, 0);
    if(readschema == NULL) {
        LOG_ERROR_0("schema json could not be read");
        if(schema_fp != NULL) {
            fclose(schema_fp);
        }
        if(config_fp != NULL) {
            fclose(config_fp);
        }
        return false;
    }
    schema = WJEOpenDocument(readschema, NULL, NULL, NULL);
    if(schema == NULL) {
        LOG_ERROR_0("schema json could not be read");
        if(readschema != NULL) {
            free(readschema);
        }
        if(schema_fp != NULL) {
            fclose(schema_fp);
        }
        if(config_fp != NULL) {
            fclose(config_fp);
        }
        return false;
    }

    // Validating config against schema
    bool result = false;
    if(schema != NULL && json != NULL) {
        result = validate_json(schema, json);
    }

    // Close schema validation related documents
    if(json != NULL) {
        WJECloseDocument(json);
    }
    if(schema != NULL) {
        WJECloseDocument(schema);
    }
    if(readjson != NULL) {
        WJRCloseDocument(readjson);
    }
    if(readschema != NULL) {
        WJRCloseDocument(readschema);
    }

    // Closing json config & schema file pointers
    if(config_fp != NULL) {
        fclose(config_fp);
    }
    if(schema_fp != NULL) {
        fclose(schema_fp);
    }

    if (!result) {
        // Clean up and return if failure
        if(readjson != NULL) {
            free(readjson);
        }
        if(json != NULL) {
            free(json);
        }
        if(readschema != NULL) {
            free(readschema);
        }
        if(schema != NULL) {
            free(schema);
        }
        clean_up();
        return false;
    }
    return true;
}

int main(int argc, char** argv) {
    signal(SIGINT, signal_callback_handler);
    signal(SIGABRT, signal_callback_handler);
    signal(SIGTERM, signal_callback_handler);

    try {
        if (argc >= 2) {
            usage(argv[0]);
            return -1;
        }

        char* str_app_name = NULL;
        str_app_name = getenv("AppName");
        if (str_app_name == NULL) {
            throw "\"AppName\" env not set";
        }

        get_config_mgr(str_app_name);

        char* str_log_level = NULL;
        log_lvl_t log_level = LOG_LVL_ERROR;  // default log level is `ERROR`

        str_log_level = getenv("C_LOG_LEVEL");
        if (str_log_level == NULL) {
            throw "\"C_LOG_LEVEL\" env not set";
        } else {
            if (strncmp(str_log_level, "DEBUG", 5) == 0) {
                log_level = LOG_LVL_DEBUG;
            } else if (strncmp(str_log_level, "INFO", 5) == 0) {
                log_level = LOG_LVL_INFO;
            } else if (strncmp(str_log_level, "WARN", 5) == 0) {
                log_level = LOG_LVL_WARN;
            } else if (strncmp(str_log_level, "ERROR", 5) == 0) {
                log_level = LOG_LVL_ERROR;
        }
        set_log_level(log_level);
        }

        // Get the configuration from the configuration manager
        char config_key[MAX_CONFIG_KEY_LENGTH];
        snprintf(config_key, MAX_CONFIG_KEY_LENGTH, "/%s/config",
                 str_app_name);

        g_va_config = g_config_mgr->get_config(config_key);
        LOG_DEBUG("App config: %s", g_va_config);

        // Validating config against schema
        if (!validate_config(g_va_config)) {
            return -1;
        }

        LOG_DEBUG("Registering watch on config key: %s", config_key);
        g_config_mgr->register_watch_key(config_key, on_change_config_callback);
        va_initialize(g_va_config, str_app_name);

        std::mutex mtx;

        while (true) {
            std::unique_lock<std::mutex> lk(mtx);
            g_err_cv.wait(lk);
            if (g_cfg_change.load()) {
                va_initialize(g_va_config, str_app_name);
                g_cfg_change.store(false);
            } else {
                break;
            }
        }

        clean_up();
    } catch(const char *err) {
        LOG_ERROR("Exception occurred: %s", err);
        clean_up();
    } catch(const std::exception& e) {
        LOG_ERROR("Exception occurred: %s", e.what());
        clean_up();
    } catch(...) {
        LOG_ERROR("Exception occurred in Video Analytics");
        clean_up();
    }
    return -1;
}
