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


class Defect:
    """Defect class
    """
    def __init__(self, defect_class, tl, br):
        """Constructor for defects class which identifies the defect.

        :param defect_class: String representation of the defect
        :type defect_class: str
        :param tl: Top left (x, y) tuple for bounding box
        :type tl: tuple
        :param br: Bottom right (x, y) tuple for bounding box
        :type br: tuple
        """

        self.defect_class = int(defect_class)
        self.tl_x = int(tl[0])
        self.tl_y = int(tl[1])
        self.br_x = int(br[0])
        self.br_y = int(br[1])

    @property
    def tl(self):
        """Helper property for top left (x, y) tuple of the bounding box

        :return: Top left (x, y) tuple for bounding box
        :rtype: tuple
        """
        return (self.tl_x, self.tl_y)

    @property
    def br(self):
        """Helper property for bottom right (x, y) tuple of the bounding box

        :return: Bottom right (x, y) tuple for bounding box
        :rtype: tuple
        """
        return (self.br_x, self.br_y)

    def __repr__(self):
        return '<Defect(defect_class={0}, tl={1}, br={2})>'.format(
                self.defect_class, self.tl, self.br)
