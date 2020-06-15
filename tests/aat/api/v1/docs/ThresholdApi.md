# client.ThresholdApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**delete_threshold_result**](ThresholdApi.md#delete_threshold_result) | **DELETE** /threshold-results/{id} | Delete the threshold result


# **delete_threshold_result**
> delete_threshold_result(id)

Delete the threshold result

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.ThresholdApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete the threshold result
    api_instance.delete_threshold_result(id)
except ApiException as e:
    print("Exception when calling ThresholdApi->delete_threshold_result: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

void (empty response body)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

