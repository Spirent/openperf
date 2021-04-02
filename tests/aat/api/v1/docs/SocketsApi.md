# client.SocketsApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**get_socket_stats**](SocketsApi.md#get_socket_stats) | **GET** /sockets/{id} | Get a socket&#39;s statistics
[**list_socket_stats**](SocketsApi.md#list_socket_stats) | **GET** /sockets | List network socket statistics


# **get_socket_stats**
> SocketStats get_socket_stats(id)

Get a socket's statistics

Return a socket's statistics by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.SocketsApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a socket's statistics
    api_response = api_instance.get_socket_stats(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling SocketsApi->get_socket_stats: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**SocketStats**](SocketStats.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_socket_stats**
> list[SocketStats] list_socket_stats()

List network socket statistics

The `sockets` endpoint returns statistics for all network sockets that are known by the stack. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.SocketsApi()

try:
    # List network socket statistics
    api_response = api_instance.list_socket_stats()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling SocketsApi->list_socket_stats: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[SocketStats]**](SocketStats.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

