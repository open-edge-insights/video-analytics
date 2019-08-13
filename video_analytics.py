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

from distutils.util import strtobool
from libs.base_classifier import load_classifier
from libs.ConfigManager import ConfigManager
from libs.log import configure_logging, LOG_LEVELS
from publisher import Publisher
from subscriber import Subscriber

# Etcd paths
CONFIG_KEY_PATH = "/config"


class VideoAnalytics:

    def __init__(self):
        """Get the video frames from messagebus, classify and add the results
        back to the messagebus
        """
        self.log = logging.getLogger(__name__)
        self.profiling = bool(strtobool(os.environ['PROFILING']))
        self.dev_mode = bool(strtobool(os.environ["DEV_MODE"]))
        self.app_name = os.environ["AppName"]
        conf = {
            "certFile": "",
            "keyFile": "",
            "trustFile": ""
        }

        cfg_mgr = ConfigManager()
        self.config_client = cfg_mgr.get_config_client("etcd", conf)
        self._read_classifier_config()
        self.config_client.RegisterDirWatch("/{0}/".format(self.app_name),
                                            self._on_change_config_callback)

    def _read_classifier_config(self):
        """Read Classifier's config data from ETCD.
        """
        config = self.config_client.GetConfig("/{0}{1}".format(
            self.app_name, CONFIG_KEY_PATH))

        self.classifier_config = json.loads(config)

    def _print_config(self):
        """Prints the config data of Classifier.
        """
        self.log.info('classifier config: {}'.format(self.classifier_config))

    def start(self):
        """ Start the Video Analytics.
        """
        log_msg = "======={} {}======="
        self.log.info(log_msg.format("Starting", self.app_name))

        self._print_config()

        queue_size = self.classifier_config["queue_size"]
        classifier_input_queue = \
            queue.Queue(maxsize=queue_size)

        classifier_output_queue = \
            queue.Queue(maxsize=queue_size)

        self.publisher = Publisher(classifier_output_queue,
                                   self.config_client, self.dev_mode)
        self.publisher.start()

        self.classifier = load_classifier(self.classifier_config["name"],
                                          self.classifier_config,
                                          classifier_input_queue,
                                          classifier_output_queue)
        self.classifier.start()

        self.subscriber = Subscriber(classifier_input_queue,
                                     self.config_client, self.dev_mode)
        self.subscriber.start()
        self.log.info(log_msg.format("Started", self.app_name))

    def stop(self):
        """ Stop the Video Analytics.
        """
        log_msg = "======={} {}======="
        self.log.info(log_msg.format("Stopping", self.app_name))
        self.subscriber.stop()
        self.classifier.stop()
        self.publisher.stop()
        self.log.info(log_msg.format("Stopped", self.app_name))

    def _on_change_config_callback(self, key, value):
        """Callback method to be called by etcd

        :param key: Etcd key
        :type key: str
        :param value: Etcd value
        :type value: str
        """
        try:
            self._read_classifier_config()
            self.stop()
            self.start()
        except Exception as ex:
            self.log.exception(ex)


def parse_args():
    """Parse command line arguments

    :return: Object of Argument Parser
    :rtype: object
    """
    parser = argparse.ArgumentParser()

    parser.add_argument('--log', choices=LOG_LEVELS.keys(), default='INFO',
                        help='Logging level (df: INFO)')
    parser.add_argument('--log-dir', dest='log_dir', default='logs',
                        help='Directory to for log files')

    return parser.parse_args()


def main():
    """Main method
    """
    # Parse command line arguments
    args = parse_args()

    currentDateTime = str(datetime.datetime.now())
    listDateTime = currentDateTime.split(" ")
    currentDateTime = "_".join(listDateTime)
    logFileName = 'videoanalytics_' + currentDateTime + '.log'

    # Creating log directory if it does not exist
    if not os.path.exists(args.log_dir):
        os.mkdir(args.log_dir)

    log = configure_logging(args.log.upper(), logFileName, args.log_dir,
                            __name__)

    va = VideoAnalytics()

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
