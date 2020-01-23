# `VideoAnalytics Module`

The VideoAnalytics module is mainly responsibly for running the classifier UDFs
and doing the required inferencing on the chosen Intel(R) Hardware
(CPU, GPU, VPU, HDDL) using openVINO.

The high level logical flow of VideoAnalytics pipeline is as below:

1. App reads the application configuration via EIS Configuration Manager which
   has details of `encoding` and `udfs`.
2. App gets the msgbus endpoint configuration from system environment.
3. Based on above two configurations, app subscribes to the published topic/stream
   coming from VideoIngestion module.
4. The frames received in the subscriber are passed onto one or more chained
   native/python UDFs for running inferencing and doing any post-processing as
   required. One can refer [UDFs README](../common/udfs/README.md) for more details
5. The frames coming out of chained udfs are published on the different topic/stream
   on EIS MessageBus.

## `Configuration`

---
**NOTE**:

* The `max_jobs`, `max_workers` and `udfs` are configuration keys related to udfs.
  For more details on udf configuration, please visit
  [../common/udfs/README.md](../common/udfs/README.md)
* For details on Etcd and MessageBus endpoint configuration, visit
  [Etcd_and_MsgBus_Endpoint_Configuration](../Etcd_and_MsgBus_Endpoint_Configuration.md).
* In case the VideoAnalytics container found to be consuming a lot of memory, then one of the suspects could be that Algo processing is slower than the frame ingestion rate. Hence a lot of frames are occupying RAM waiting to be processed. In that case user can reduce the high watermark value to acceptable lower number so that RAM consumption will be under control and stay stabilzed. The exact config parameter is called **ZMQ_RECV_HWM** present in [docker-compose.yml](../docker_setup/docker-compose.yml). This config is also present in other types of container, hence user can tune them to control the memory bloat if applicable. The config snippet is pasted below:
```bash
      ZMQ_RECV_HWM: "1000"
```
---

All the app module configuration are added into distributed
key-value data store under `AppName` env as mentioned in the
environment section of this app's service definition in docker-compose.

If `AppName` is `VideoAnalytics`, then the app's config would be fetched from
`/VideoAnalytics/config` key via EIS Configuration Manager.
Below is the JSON schema for app's config:

```javascript
{
  "type": "object",
  "additionalProperties": false,
  "required": [
    "encoding",
    "udfs"
  ],
  "properties": {
    "encoding": {
      "description": "Encoding object",
      "type": "object",
      "required": [
        "type",
        "level"
      ],
      "properties": {
        "type": {
          "description": "Encoding type",
          "type": "string",
          "enum": [
              "jpeg",
              "png"
            ]
        },
        "level": {
          "description": "Encoding value",
          "type": "integer",
          "default": 0
        }
      }
    },
    "max_jobs": {
      "description": "Number of queued UDF jobs",
      "type": "integer",
      "default": 20
    },
    "max_workers": {
      "description": "Number of threads acting on queued jobs",
      "type": "integer",
      "default": 4
    },
    "udfs": {
      "description": "Array of UDF config objects",
      "type": "array",
      "items": [
        {
          "description": "UDF config object",
          "type": "object",
          "properties": {
            "type": {
              "description": "UDF type",
              "type": "string",
              "enum": [
                "native",
                "python"
              ]
            },
            "name": {
              "description": "Unique UDF name",
              "type": "string"
            },
            "device": {
              "description": "Device on which inference occurs",
              "type": "string",
              "enum": [
                "CPU",
                "GPU",
                "HDDL",
                "MYRIAD"
              ]
            }
          },
          "additionalProperties": true,
          "required": [
            "type",
            "name",
            "device"
          ]
        }
      ]
    }
  }
}
```

One can use [JSON validator tool](https://www.jsonschemavalidator.net/) for
validating the app configuration against the above schema.

## `Installation`

* Follow [provision/README.md](../README#provision-eis.md) for EIS provisioning

* Run VideoAnalytics

  Present working directory to try out below commands is: `[repo]/VideoAnalytics`

    1. Build and Run VideoAnalytics as container

        ```sh
         $ cd [repo]/build
         $ docker-compose up --build ia_video_analytics
        ```

    2. For any updates to EIS VideoAnalytics config key in distributed key-value
       store using UI's like `EtcdKeeper` or programmatically, the container
       restarts to pick the new changes.
