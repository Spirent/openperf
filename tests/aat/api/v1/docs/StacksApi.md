# client.StacksApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**get_stack**](StacksApi.md#get_stack) | **GET** /stacks/{id} | Get a stack
[**list_stacks**](StacksApi.md#list_stacks) | **GET** /stacks | List stacks


# **get_stack**
> Stack get_stack(id)

Get a stack

Returns a stack, by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.StacksApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a stack
    api_response = api_instance.get_stack(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling StacksApi->get_stack: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**Stack**](Stack.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_stacks**
> list[Stack] list_stacks()

List stacks

The `stacks` endpoint returns all TCP/IP stacks.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.StacksApi()

try:
    # List stacks
    api_response = api_instance.list_stacks()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling StacksApi->list_stacks: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[Stack]**](Stack.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

