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

import threading
import logging
import os
import json
import time
from concurrent.futures import ThreadPoolExecutor
from util.msgbusutil import MsgBusUtil
import eis.msgbus as mb
from distutils.util import strtobool


class Publisher:

    def __init__(self, classifier_output_queue, topic, config_client,
                 dev_mode):
        """Publisher will get the classified data from the classified queue and
           send it to EIS Message Bus

        :param classifier_output_queue: Input queue for publisher (has
                                        (metadata,frame) tuple data entries)
        :type classifier_output_queue: queue
        :param topic: Topic on which data is published
        :type topic: String
        :param config_client: Used to get keys value from ETCD.
        :type config_client: Class Object
        :param dev_mode: To check whether it is running in production mode
                         or development
        :type dev_mode: Boolean
        """
        self.log = logging.getLogger(__name__)
        self.classifier_output_queue = classifier_output_queue
        self.stop_ev = threading.Event()
        self.topic = topic
        self.config_client = config_client
        self.dev_mode = dev_mode
        self.profiling = bool(strtobool(os.environ['PROFILING_MODE']))

    def start(self):
        """Starts the publisher thread(s)
        """
        self.publisher_threadpool = \
            ThreadPoolExecutor(max_workers=len(self.topic))
        subscribers = os.environ['Clients'].split(",")
        msgbus_cfg = \
            MsgBusUtil.get_messagebus_config(self.topic, "pub", subscribers,
                                             self.config_client,
                                             self.dev_mode)

        self.publisher_threadpool.submit(self.publish, msgbus_cfg)

    def publish(self, msgbus_cfg):
        """Send the data to the publish topic

        :param topic: Publishers's topic name
        :type topic: str
        :param msgbus_cfg: Topic msgbus_cfg
        :type msgbus_cfg: str
        """
        publisher = None
        try:
            self.log.debug("config:{}".format(msgbus_cfg))
            msgbus = mb.MsgbusContext(msgbus_cfg)
            publisher = msgbus.new_publisher(self.topic)
            thread_id = threading.get_ident()
            log_msg = "Thread ID: {} {} with topic:{}"
            self.log.info(log_msg.format(thread_id, "started",
                                         self.topic))
            self.log.info("Publishing to topic: {}...".format(self.topic))
            while not self.stop_ev.is_set():
                metadata, frame = self.classifier_output_queue.get()
                if 'defects' in metadata:
                    metadata['defects'] = \
                        json.dumps(metadata['defects'])
                if 'display_info' in metadata:
                    metadata['display_info'] = \
                        json.dumps(metadata['display_info'])

                if self.profiling is True:
                    metadata['ts_va_exit'] = time.time()*1000

                publisher.publish((metadata, frame))
                self.log.debug("Published data: {} on topic: {}".format(metadata,
                                                                        self.topic))
                self.log.info("Published data on topic: {}".format(self.topic))
        except Exception as ex:
            self.log.exception('Error while publishing data:\
                            {}'.format(ex))
        finally:
            if publisher is not None:
                publisher.close()
        self.log.info(log_msg.format(thread_id, "stopped", self.topic))

    def stop(self):
        """Stops the pubscriber thread
        """
        try:
            self.stop_ev.set()
            self.publisher_threadpool.shutdown(wait=False)
        except Exception as ex:
            self.log.exception(ex)
