# client.SocketsApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**list_socket_stats**](SocketsApi.md#list_socket_stats) | **GET** /socket-stats | List network socket stats


# **list_socket_stats**
> list[SocketStats] list_socket_stats()

List network socket stats

The `sockets-stats` endpoint returns stats for all network sockets that are known by the stack. 

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
    # List network socket stats
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

