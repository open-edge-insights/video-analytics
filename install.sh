#!/bin/bash -x

# Copyright (c) 2020 Intel Corporation.

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

source ../../build/.env

#Install python dependencies
sudo -H pip3.6 install -r va_requirements.txt

if [ $DEV_MODE = "false" ]
then
	# Set the Permission
	chmod -R 777 /etc/ssl

	# create the required directories for certificate and keys on bare-metal (docker host machine).
	mkdir -p /etc/ssl/ca/
	mkdir -p /etc/ssl/streamsublib
	mkdir -p /etc/ssl/imagestore/

	#Copy CA, certificates/keys to respective directories
	cp ../../cert-tool/Certificates/ca/ca_certificate.pem /etc/ssl/ca/ca_certificate.pem

	# Copy certificates from data agent
	#docker cp ia_data_agent:/etc/ssl/grpc_int_ssl_secrets /etc/ssl/
fi
