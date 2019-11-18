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
        "queue_size": 10,
        "max_jobs": 1,
        "name": "pcb.pcb_classifier",
        "udfs": [
            {
                "name": "pcb.pcb_classifier",
                "type": "python",
                "ref_img": "common/udfs/python/pcb/ref/ref.png",
                "ref_config_roi": "common/udfs/python/pcb/ref/roi_2.json",
                "model_xml": "common/udfs/python/pcb/ref/model_2.xml",
                "model_bin": "common/udfs/python/pcb/ref/model_2.bin",
                "device": "CPU""
            }
        ]
    }
 ```
For more details on Etcd and MessageBus endpoint configuration, visit [Etcd_and_MsgBus_Endpoint_Configuration](../Etcd_and_MsgBus_Endpoint_Configuration.md).

### `Classifier config`

The Classifier (user defined function) is responsible for running classification
algorithm on the video frames received from filter.

Sample configuration for classifiers used:

1. **PCB classifier** (has to be used with `PCB Filter`)
   ```
    {
        "queue_size": 10,
        "max_jobs": 1,
        "name": "pcb.pcb_classifier",
        "udfs": [
            {
                "name": "pcb.pcb_classifier",
                "type": "python",
                "ref_img": "common/udfs/python/pcb/ref/ref.png",
                "ref_config_roi": "common/udfs/python/pcb/ref/roi_2.json",
                "model_xml": "common/udfs/python/pcb/ref/model_2.xml",
                "model_bin": "common/udfs/python/pcb/ref/model_2.bin",
                "device": "CPU""
            }
        ]
    }
    
    ```

    ----
    **NOTE**:
    The above config works for both "CPU" and "GPU" devices after setting
    appropriate `device` value. If the device in the above config is "HDDL" or
    "MYRIAD", please use the below config where the model_xml and model_bin
    files are different. Please set the "device" value appropriately based on
    the device used for inferencing.
    ```
    "udfs": [
        {
            "name": "pcb.pcb_classifier",
            "type": "python",
            "device": "HDDL"
        }
    ]

    ```
    ----

2. **Dummy classifier** (to be used when no classification needs to be done)
   ```
    "udfs": [{
        "name": "dummy",
        "type": "native",
    }
   ```

#### `Detailed description on each of the keys used`

|  Key	        | Description 	                                    | Possible Values  	                             | Required/Optional |
|---	        |---	                                            |---	                                         |---	             |
|  name 	    |   File name of the classifier	                    |  "pcb.pcb_classifier"/"dummy"                  |   Required	     |
|  type 	    |   type of classifier                              |  "python"/"python"                             |   Required	     |
|  queue_size 	|   Input and output queue size of classifier       | any number that suits the platform resources	 |  Required	     |
|  max_workers 	|   Number of threads running classification algo (Not more than 5 * number of cpu cores)	       |   any number that suits the platform resources | Optional(Default: 4) |
|  model_xml	|  xml file generated from openvino model optimizer | -                                              | Required          |
|  model_bin	|  bin file generated from openvino model optimizer | -                                              | Required          |
|  device	    |  device on which the inference occurs             | "CPU"/"GPU"/"HDDL"/"MYRIAD"                    | Required          |
|  max_jobs     |  number of queued jobs                            | Default value: 20                              | Optional          |

**Note**: The other keys used are specific to classifier usecase

## `Installation`

* Follow [provision/README.md](../README#provision-eis.md) for EIS provisioning

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
