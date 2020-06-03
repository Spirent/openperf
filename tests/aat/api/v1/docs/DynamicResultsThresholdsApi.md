# client.DynamicResultsThresholdsApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**delete_threshold**](DynamicResultsThresholdsApi.md#delete_threshold) | **DELETE** /dynamic-results/thresholds/{id} | Delete the threshold


# **delete_threshold**
> delete_threshold()

Delete the threshold

Delete the threshold by ID

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.DynamicResultsThresholdsApi()

try:
    # Delete the threshold
    api_instance.delete_threshold()
except ApiException as e:
    print("Exception when calling DynamicResultsThresholdsApi->delete_threshold: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

void (empty response body)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

