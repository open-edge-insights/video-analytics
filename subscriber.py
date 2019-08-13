# Copyright (c) 2019 Intel Corporation.

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import threading
import json
import logging
import os
import numpy as np
import cv2
from concurrent.futures import ThreadPoolExecutor
from libs.common.py.util import Util
import eis.msgbus as mb


class Subscriber:

    def __init__(self, subscriber_queue, config_client, dev_mode):
        """Subscriber subscribes to the EISMessageBus on which VideoIngestion
           is publishing

        :param subscriber_queue: subscriber's output queue (has
                                 (metadata,frame) tuple data entries)
        :type subscriber_queue: queue
        :param config_client: Used to get keys value from ETCD.
        :type config_client: Class Object
        :param dev_mode: To check whether it is running in production mode
                         or development
        :type dev_mode: Boolean
        """
        self.log = logging.getLogger(__name__)
        self.subscriber_queue = subscriber_queue
        self.stop_ev = threading.Event()
        self.config_client = config_client
        self.dev_mode = dev_mode

    def start(self):
        """Starts the subscriber thread
        """
        topics = Util.get_topics_from_env("sub")
        self.subscriber_threadpool = \
            ThreadPoolExecutor(max_workers=len(topics))
        for topic in topics:
            publisher, topic = topic.split("/")
            msgbus_cfg = Util.get_messagebus_config(topic, "sub",
                                                    publisher,
                                                    self.config_client,
                                                    self.dev_mode)

            self.subscriber_threadpool.submit(self.subscribe, topic,
                                              msgbus_cfg)

    def subscribe(self, topic, msgbus_cfg):
        """Receives the data for the subscribed topic

        :param topic: Subscribing to topic name
        :type topic: str
        :param msgbus_cfg: Topic msgbus_cfg
        :type msgbus_cfg: str
        """
        subscriber = None
        try:
            msgbus = mb.MsgbusContext(msgbus_cfg)
            subscriber = msgbus.new_subscriber(topic)
            thread_id = threading.get_ident()
            log_msg = "Thread ID: {} {} with topic:{} and msgbus_cfg:{}"
            self.log.info(log_msg.format(thread_id, "started", topic, msgbus_cfg))
            self.log.info("Subscribing to topic: {}...".format(topic))
            while not self.stop_ev.is_set():
                data = subscriber.recv()
                self.subscriber_queue.put(data)
                self.log.debug("Subscribed data: {} on topic: {} with " +
                               "config: {}...".format(data[0], topic,
                                                      msgbus_cfg))
        except Exception as ex:
            self.log.exception('Error while subscribing data:\
                            {}'.format(ex))
        finally:
            if subscriber is not None:
                subscriber.close()

        self.log.info(log_msg.format(thread_id, "stopped", topic, msgbus_cfg))

    def stop(self):
        """Stops the Subscriber thread
        """
        try:
            self.stop_ev.set()
            self.subscriber_threadpool.shutdown(wait=False)
        except Exception as ex:
            self.log.exception(ex)
