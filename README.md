# `VideoAnalytics Module`

The high level logical flow of VideoAnalytics pipeline is as below:

1. VideoAnalytics will start the messagebus publisher thread, single/multiple
   classifier threads and messagebus subscriber thread based on classifier
   configuration.
2. The subscriber thread connects to the PUB socket of messagebus on which
   the data is published by VideoIngestion and adds it to classifier
   input queue
3. Based on the classifier configuration, single or multiple classifier
   threads consume classifier input queue and processes the frames to
   add defects/display_info to metadata and add's the updated data to
   classifier output queue
4. The publisher thread reads from the classifier output queue and
   publishes it over the messagebus

## `Configuration`

All the VideoAnalytics module configuration are added into etcd (distributed
key-value data store) under `AppName` as mentioned in the
environment section of this app's service definition in docker-compose.

If `AppName` is `VideoAnalytics`, then the app's config would look like as below
 for `/VideoAnalytics/config` key in Etcd:
 ```
    {
        "name": "pcb_classifier",
        "queue_size": 10,
        "max_workers": 1,
        "ref_img": "./VideoAnalytics/classifiers/ref_pcbdemo/ref.png",
        "ref_config_roi": "./VideoAnalytics/classifiers/ref_pcbdemo/roi_2.json",
        "model_xml": "./VideoAnalytics/classifiers/ref_pcbdemo/model_2.xml",
        "model_bin": "./VideoAnalytics/classifiers/ref_pcbdemo/model_2.bin",
        "device": "CPU""
    }
 ```
For more details on Etcd and MessageBus endpoint configuration, visit [Etcd_and_MsgBus_Endpoint_Configuration](../Etcd_and_MsgBus_Endpoint_Configuration].md).


### `Messagebus Endpoints config`

The ENVs mentioned below in the environment section of this app's service in
[docker-compose.yml](../docker_setup/docker-compose.yml) are
needed by the messagebus subscriber and publisher thread to start subscribing
to the published topics data and start publishing classified results data on
new topics

```
SubTopics: "<publisher_appname>stream1,<publisher_appname>stream2"
stream1_cfg: "<protocol>,<endpoint>"
stream2_cfg: "<protocol>,<endpoint>"

PubTopics: "stream1_results, stream2_results"
stream1_results_cfg: "<protocol>,<endpoint>"
stream2_results_cfg: "<protocol>,<endpoint>"
```

> **NOTE**: If `<protocol>` is `zmq/ipc`, then `<endpoint>` has to be the
> `socket_dir_name` where unix socket files are created. If `<protocol>` is
> `zmq/tcp`, then `<endpoint>` has to be the combination of `<hostname>:<port>`.

### `Classifier config`

The Classifier (user defined function) is responsible for running classification
algorithm on the video frames received from filter.

Sample configuration for classifiers used:

1. **PCB classifier** (has to be used with `PCB Filter`)
   ```
   {
        "name": "pcb_classifier",
        "queue_size": 10,
        "max_workers": 1,
        "ref_img": "./VideoAnalytics/classifiers/ref_pcbdemo/ref.png",
        "ref_config_roi": "./VideoAnalytics/classifiers/ref_pcbdemo/roi_2.json",
        "model_xml": "./VideoAnalytics/classifiers/ref_pcbdemo/model_2.xml",
        "model_bin": "./VideoAnalytics/classifiers/ref_pcbdemo/model_2.bin",
        "device": "CPU"
    }
    ```

2. **Classification sample classifier** (has to be used with `Bypass Filter`)
   ```
    {
        "name": "sample_classification_classifier",
        "queue_size": 10,
        "max_workers": 1,
        "model_xml": "./VideoAnalytics/classifiers/ref_classification/squeezenet1.1_FP32.xml",
        "model_bin": "./VideoAnalytics/classifiers/ref_classification/squeezenet1.1_FP32.bin",
        "labels": "./VideoAnalytics/classifiers/ref_classification/squeezenet1.1.labels",
        "device": "CPU"
    }
   ```
3. **Dummy classifier** (to be used when no classification needs to be done)
   ```
    {
        "name": "dummy_classifier",
        "queue_size": 10,
        "max_workers": 1
    }
   ```

**Sample classifiers code**

|  File	                              | Description 	                             | Link  	                                                |
|---	                              |---	                                         |---	                                                    |
| base_classifier.py                  | Base class for all classifiers               | [Link](..libs/base_classifier.py)                        |
| pcb_classifier.py                   | PCB Demo classifier for detecting defects    | [Link](classifiers/pcb_classifier.py)                    |
| sample_classification_classifier.py | Sample classification classifier             | [Link](classifiers/sample_classification_classifier.py)  |
| dummy_classifier.py                 | Doesn't do any classification                | [Link](classifiers/dummy_classifier.py)                  |


#### `Detailed description on each of the keys used`

|  Key	        | Description 	                                    | Possible Values  	                             | Required/Optional |
|---	        |---	                                            |---	                                         |---	             |
|  name 	    |   File name of the classifier	                    |  "pcb_classfier"/"sample_classification_classifier"/"dummy_classifier" |   Required	|
|  queue_size 	|   Input and output queue size of classifier       | any number that suits the platform resources	 |  Required	     |
|  max_workers 	|   Number of threads running classification algo (Not more than 5 * number of cpu cores)	    |   any number that suits the platform resources | Required |
|  model_xml	|  xml file generated from openvino model optimizer | -                                              | Required          |
|  model_bin	|  bin file generated from openvino model optimizer | -                                              | Required          |
|  device	    |  device on which the inference occurs             | "CPU"/"GPU"/"HDDL"/"MYRIAD"                    | Required          |

**Note**: The other keys used are specific to classifier usecase

## `Installation`

* Follow [provision/README.md](../docker_setup/provision/README.md) for EIS provisioning

* Run VideoAnalytics

  Present working directory to try out below commands is: `[repo]/VideoAnalytics`

    1. Build and Run VideoAnalytics as container
        ```
        $ cd [repo]/docker_setup
        $ docker-compose up --build ia_video_analytics
        ```
    2. For any updates to EIS VideoAnalytics config key in `etcd` using UI's
       like `EtcdKeeper` or programmatically, the container restarts to pick the
       new changes.
