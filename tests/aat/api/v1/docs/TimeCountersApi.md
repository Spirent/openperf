# client.TimeCountersApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**get_time_counter**](TimeCountersApi.md#get_time_counter) | **GET** /time-counters/{id} | Get a time counter
[**list_time_counters**](TimeCountersApi.md#list_time_counters) | **GET** /time-counters | List time counters


# **get_time_counter**
> TimeCounter get_time_counter(id)

Get a time counter

Returns a time counter, by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TimeCountersApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a time counter
    api_response = api_instance.get_time_counter(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TimeCountersApi->get_time_counter: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**TimeCounter**](TimeCounter.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_time_counters**
> list[TimeCounter] list_time_counters()

List time counters

The `time-counters` endpoint returns all local time counters that are available for measuring the passage of time.  This list is for informational purposes only. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TimeCountersApi()

try:
    # List time counters
    api_response = api_instance.list_time_counters()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TimeCountersApi->list_time_counters: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[TimeCounter]**](TimeCounter.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

