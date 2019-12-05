# client.TimeSourcesApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**create_time_source**](TimeSourcesApi.md#create_time_source) | **POST** /time-sources | Register a time source for time syncing.
[**delete_time_source**](TimeSourcesApi.md#delete_time_source) | **DELETE** /time-sources/{id} | Delete a time source
[**get_time_source**](TimeSourcesApi.md#get_time_source) | **GET** /time-sources/{id} | Get a time source
[**list_time_sources**](TimeSourcesApi.md#list_time_sources) | **GET** /time-sources | List reference clocks


# **create_time_source**
> TimeSource create_time_source(timesource)

Register a time source for time syncing.

Registers a new time source for time syncing.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TimeSourcesApi()
timesource = client.TimeSource() # TimeSource | New time source

try:
    # Register a time source for time syncing.
    api_response = api_instance.create_time_source(timesource)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TimeSourcesApi->create_time_source: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **timesource** | [**TimeSource**](TimeSource.md)| New time source | 

### Return type

[**TimeSource**](TimeSource.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **delete_time_source**
> delete_time_source(id)

Delete a time source

Deletes an existing time source. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TimeSourcesApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete a time source
    api_instance.delete_time_source(id)
except ApiException as e:
    print("Exception when calling TimeSourcesApi->delete_time_source: %s\n" % e)
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

# **get_time_source**
> TimeSource get_time_source(id)

Get a time source

Get a time source, by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TimeSourcesApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a time source
    api_response = api_instance.get_time_source(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TimeSourcesApi->get_time_source: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**TimeSource**](TimeSource.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_time_sources**
> list[TimeSource] list_time_sources()

List reference clocks

The `time-sources` endpoint returns all network clock sources that are used for syncing the local time. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TimeSourcesApi()

try:
    # List reference clocks
    api_response = api_instance.list_time_sources()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TimeSourcesApi->list_time_sources: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[TimeSource]**](TimeSource.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

