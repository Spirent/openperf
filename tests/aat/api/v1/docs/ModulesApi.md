# client.ModulesApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**get_module**](ModulesApi.md#get_module) | **GET** /modules/{id} | Get a module
[**list_modules**](ModulesApi.md#list_modules) | **GET** /modules | List Modules


# **get_module**
> Module get_module(id)

Get a module

Returns a module, by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.ModulesApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a module
    api_response = api_instance.get_module(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling ModulesApi->get_module: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**Module**](Module.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_modules**
> list[Module] list_modules()

List Modules

The `modules` endpoint returns all loaded modules.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.ModulesApi()

try:
    # List Modules
    api_response = api_instance.list_modules()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling ModulesApi->list_modules: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[Module]**](Module.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

