# Copyright (c) 2019 Intel Corporation.

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import os
import datetime
import queue
import signal
import argparse
import json
import logging
import threading
import sys

from distutils.util import strtobool
from libs.base_classifier import load_classifier
from libs.ConfigManager import ConfigManager
from util.log import configure_logging, LOG_LEVELS
from util.msgbusutil import MsgBusUtil
from publisher import Publisher
from subscriber import Subscriber

# Etcd paths
CONFIG_KEY_PATH = "/config"


class VideoAnalytics:

    def __init__(self, dev_mode, config_client):
        """Get the video frames from messagebus, classify and add the results
        back to the messagebus

        :param dev_mode: indicates whether it's dev or prod mode
        :type dev_mode: bool
        :param config_client: distributed store config client
        :type config_client: config client object
        """
        self.log = logging.getLogger(__name__)
        self.profiling = bool(strtobool(os.environ['PROFILING']))
        self.dev_mode = dev_mode
        self.app_name = os.environ["AppName"]
        self.config_client = config_client
        self._read_classifier_config()
        self.config_client.RegisterDirWatch("/{0}/".format(self.app_name),
                                            self._on_change_config_callback)

    def _read_classifier_config(self):
        """Reads classifier config from distributed data store
        """
        config = self.config_client.GetConfig("/{0}{1}".format(
            self.app_name, CONFIG_KEY_PATH))

        self.classifier_config = json.loads(config)

    def _print_classifier_config(self):
        """Prints classifier config
        """
        self.log.info('classifier config: {}'.format(self.classifier_config))

    def start(self):
        """Starts the Video Analytics pipeline
        """
        log_msg = "======={} {}======="
        self.log.info(log_msg.format("Starting", self.app_name))

        pub_topic = MsgBusUtil.get_topics_from_env("pub")
        sub_topic = MsgBusUtil.get_topics_from_env("sub")

        if len(pub_topic) > 1:
            self.log.error("More than one publish topic in {} " +
                           "is not allowed".format(self.app_name))
            sys.exit(1)

        if len(sub_topic) > 1:
            self.log.error("More than one subscribe topic in {} " +
                           "is not allowed".format(self.app_name))
            sys.exit(1)

        self._print_classifier_config()

        queue_size = self.classifier_config["queue_size"]
        classifier_input_queue = \
            queue.Queue(maxsize=queue_size)

        classifier_output_queue = \
            queue.Queue(maxsize=queue_size)

        pub_topic = pub_topic[0]
        sub_topic = sub_topic[0]

        self.publisher = Publisher(classifier_output_queue, pub_topic,
                                   self.config_client, self.dev_mode)
        self.publisher.start()

        self.classifier = load_classifier(self.classifier_config["name"],
                                          self.classifier_config,
                                          classifier_input_queue,
                                          classifier_output_queue)
        self.classifier.start()

        self.subscriber = Subscriber(classifier_input_queue, sub_topic,
                                     self.config_client, self.dev_mode)
        self.subscriber.start()
        self.log.info(log_msg.format("Started", self.app_name))

    def stop(self):
        """Stops the Video Analytics pipeline"""
        log_msg = "======={} {}======="
        self.log.info(log_msg.format("Stopping", self.app_name))
        os._exit(1)

    def _on_change_config_callback(self, key, value):
        """Callback method to be called by etcd

        :param key: distributed store key
        :type key: str
        :param value: changed value
        :type value: str
        """
        try:
            self._read_classifier_config()
            self.stop()
            self.start()
        except Exception as ex:
            self.log.exception(ex)


def main():
    """Main method
    """
    dev_mode = bool(strtobool(os.environ["DEV_MODE"]))

    conf = {
        "certFile": "",
        "keyFile": "",
        "trustFile": ""
    }
    if not dev_mode:
        conf = {
            "certFile": "/run/secrets/etcd_VideoAnalytics_cert",
            "keyFile": "/run/secrets/etcd_VideoAnalytics_key",
            "trustFile": "/run/secrets/ca_etcd"
        }

    cfg_mgr = ConfigManager()
    config_client = cfg_mgr.get_config_client("etcd", conf)

    log = configure_logging(os.environ['PY_LOG_LEVEL'].upper(),
                            __name__)

    va = VideoAnalytics(dev_mode, config_client)

    def handle_signal(signum, frame):
        log.info('Video Ingestion program killed...')
        va.stop()

    signal.signal(signal.SIGTERM, handle_signal)

    try:
        va.start()
    except Exception as ex:
        log.exception('Exception: {}'.format(ex))
        va.stop()


if __name__ == '__main__':
    main()
