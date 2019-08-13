#!/usr/bin/env python3.6

"""
Copyright (c) 2019 Intel Corporation.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""

import os
import unittest
import datetime
import time
from Util.log import configure_logging
from DataAnalytics.VideoAnalytics.VideoAnalytics import VideoAnalytics


class VideoAnalyticsTest(unittest.TestCase):
    """Test cases for PCBDemo module"""

    @classmethod
    def setUpClass(cls):
        current_date_time = str(datetime.datetime.now())
        list_date_time = current_date_time.split(" ")
        current_date_time = "_".join(list_date_time)

        log_dir = 'testVideoAnalytics'
        if not os.path.exists(log_dir):
            os.mkdir(log_dir)
        log_file_name = 'videoanalyticsApp_' + current_date_time + '.log'

        base_config_path = '../../docker_setup/config/algo_config'
        cls.log = configure_logging('DEBUG', log_file_name, log_dir, __name__)
        cls.config = base_config_path + '/factory_pcbdemo.json'
        cls.multi_cam_config =\
            base_config_path + '/factory_multi_cam.json'

    def test_main(self):
        app = VideoAnalytics(self.config, self.log)
        self.assertNotEqual(app.data_handler, None)
        self.assertNotEqual(app.stream_sub_lib, None)

        app.main()

        # Since main is infinite call
        # lets this run for 5 second and unsubscribe to influx to stop the test
        time.sleep(5)
        app.stream_sub_lib.deinit()

    def test_main_multi_cam(self):
        app = VideoAnalytics(self.multi_cam_config, self.log)
        self.assertNotEqual(app.data_handler, None)
        self.assertNotEqual(app.stream_sub_lib, None)

        app.main()

        # Since main is infinite call
        # lets this run for 5 second and unsubscribe to influx to stop the test
        time.sleep(5)
        app.stream_sub_lib.deinit()


if __name__ == '__main__':
    unittest.main()

# To run the test, use the following command
# DataAnalytics/VideoAnalytics$ python3.6 test/TestVideoAnalytics.py
