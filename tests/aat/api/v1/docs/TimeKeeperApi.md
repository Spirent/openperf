# client.TimeKeeperApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**get_time_keeper**](TimeKeeperApi.md#get_time_keeper) | **GET** /time-keeper | Get a time keeper.


# **get_time_keeper**
> TimeKeeper get_time_keeper()

Get a time keeper.

Returns the system time keeper, aka clock.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TimeKeeperApi()

try:
    # Get a time keeper.
    api_response = api_instance.get_time_keeper()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TimeKeeperApi->get_time_keeper: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**TimeKeeper**](TimeKeeper.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

