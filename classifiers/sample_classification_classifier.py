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
import logging
import cv2
import numpy as np
import json
from time import time

from VideoAnalytics.classifiers.defect import Defect
from VideoAnalytics.classifiers.display_info import DisplayInfo
from libs.base_classifier import BaseClassifier
from openvino.inference_engine import IENetwork, IEPlugin


class Classifier(BaseClassifier):
    """Classifier object
    """

    def __init__(self, classifier_config, input_queue, output_queue):
        """Constructor of Sample classification algorithm

        :param classifier_config: Configuration object for the classifier
        :type classifier_config: dict
        :param input_queue: input queue for classifier
        :type input_queue: queue
        :param output_queue: output queue of classifier
        :type output_queue: queue
        """
        super().__init__(classifier_config, input_queue, output_queue)
        self.log = logging.getLogger('SAMPLE CLASSIFICATION')
        labels = classifier_config["labels"]
        model_xml = classifier_config["model_xml"]
        model_bin = classifier_config["model_bin"]
        device = classifier_config["device"]

        # Assert all input parameters exist
        assert os.path.exists(model_xml), \
            'Classification model xml file missing: {}'.format(model_xml)
        assert os.path.exists(model_bin), \
            'Classification model bin file missing: {}'.format(model_bin)
        assert os.path.exists(labels), \
            'Labels mapping file missing: {}'.format(labels)

        # Load labels file associated with the model
        with open(labels, 'r') as f:
            self.labels_map = [x.split(sep=' ', maxsplit=1)[-1].strip() for x
                               in f]

        # Load OpenVINO model
        self.plugin = IEPlugin(device=device.upper(), plugin_dirs="")
        self.log.debug("Loading network files:\n\t{}\n\t{}".format(model_xml,
                                                                   model_bin))
        self.net = IENetwork.from_ir(model=model_xml, weights=model_bin)
        if device.upper() == "CPU":
            supported_layers = self.plugin.get_supported_layers(self.net)
            not_supported_layers = [l for l in self.net.layers.keys() if l not
                                    in supported_layers]
            if len(not_supported_layers) != 0:
                self.log.debug('ERROR: Following layers are not supported by \
                                the plugin for specified device {}:\n \
                                {}'.format(self.plugin.device,
                                           ', '.join(not_supported_layers)))

        assert len(self.net.inputs.keys()) == 1, \
            'Sample supports only single input topologies'
        assert len(self.net.outputs) == 1, \
            'Sample supports only single output topologies'

        self.input_blob = next(iter(self.net.inputs))
        self.output_blob = next(iter(self.net.outputs))
        self.net.batch_size = 1  # change to enable batch loading
        self.exec_net = self.plugin.load(network=self.net)

    # Main classification algorithm
    def classify(self):
        """Sample classification algorithm to classify input images.
        This sample algorithm classifies input images, hence no defect
        information in generated. The trigger algorithm associated with
        with this classifier is "bypass_filter" or "no_filter" which selects
        each input image for classification.
        """

        while True:
            metadata, frame = self.input_queue.get()
            self.log.debug("classify data: metadata:{}".format(metadata))

            # Convert the buffer into np array.
            np_buffer = np.frombuffer(frame, dtype=np.uint8)
            if 'encoding_type' and 'encoding_level' in metadata:
                reshape_frame = np.reshape(np_buffer, (np_buffer.shape))
                reshape_frame = cv2.imdecode(reshape_frame, 1)
            else:
                reshape_frame = np.reshape(np_buffer, (int(metadata["height"]),
                                                       int(metadata["width"]),
                                                       int(metadata["channel"])
                                                       ))

            # Read and preprocess input images
            n, c, h, w = self.net.inputs[self.input_blob].shape
            images = np.ndarray(shape=(n, c, h, w))
            for i in range(n):
                if reshape_frame.shape[:-1] != (h, w):
                    self.log.debug('Image is resized from {} to {}'.format(
                                reshape_frame.shape[:-1], (w, h)))
                    reshape_frame = cv2.resize(reshape_frame, (w, h))
                # Change layout from HWC to CHW
                reshape_frame = reshape_frame.transpose((2, 0, 1))
                images[i] = reshape_frame
            self.log.debug('Batch size is {}'.format(n))

            # Start sync inference
            infer_time = []
            t0 = time()
            res = self.exec_net.infer(inputs={self.input_blob: images})
            infer_time.append((time() - t0)*1000)
            self.log.info('Average running time of one iteration: {} ms'.
                          format(np.average(np.asarray(infer_time))))

            # Display information for visualizer
            d_info = []

            # Processing output blob
            self.log.debug('Processing output blob')
            res = res[self.output_blob]
            self.log.info("Top 5 results :")

            for i, probs in enumerate(res):
                probs = np.squeeze(probs)
                top_ind = np.argsort(probs)[-5:][::-1]
                for id in top_ind:
                    det_label = self.labels_map[id] \
                            if self.labels_map else '#{}'.format(id)
                    self.log.info('prob: {:.7f}, label: {}'.format(probs[id],
                                                                   det_label))
                    # LOW priority information string to be displayed with
                    # frame
                    disp_info = DisplayInfo('prob: {:.7f}, label: {} \
                                            '.format(probs[id], det_label), 0)
                    d_info.append(disp_info)

            display_info = []
            for d in d_info:
                display_info.append({
                    'info': d.info,
                    'priority': d.priority
                })
            metadata["display_info"] = display_info
            self.output_queue.put((metadata, frame))
            self.log.debug("metadata: {} added to classifier output queue".
                           format(metadata))
