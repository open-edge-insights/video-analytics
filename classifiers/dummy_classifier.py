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

import logging
from libs.base_classifier import BaseClassifier


class Classifier(BaseClassifier):
    """Dummy classifier which never returns any defects. This is meant to aid
    pure data collection without running any classification on ingested video
    frames.
    """
    def __init__(self, classifier_config, input_queue, output_queue):
        """Constructor to initialize classifier object

        :param BaseClassifier: Base class for all classifier classes
        :type BaseClassifier: Class
        :param classifier_config: Configuration object for the classifier
        :type classifier_config: dict
        :param input_queue: input queue for classifier
        :type input_queue: queue
        :param output_queue: output queue of classifier
        :type output_queue: queue
        :return: Classification object
        :rtype: Object
        """
        super().__init__(classifier_config, input_queue, output_queue)
        self.log = logging.getLogger('DUMMY_CLASSIFIER')

    def classify(self):
        """Classify the given image.
        """
        while True:
            metadata, frame = self.input_queue.get()
            self.output_queue.put((metadata, frame))
            self.log.debug("metadata: {} added to classifier output queue".
                           format(metadata))
