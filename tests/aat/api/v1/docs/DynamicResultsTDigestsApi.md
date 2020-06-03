# client.DynamicResultsTDigestsApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**delete_t_digest**](DynamicResultsTDigestsApi.md#delete_t_digest) | **DELETE** /dynamic-results/t-digests/{id} | Delete the T-Digest


# **delete_t_digest**
> delete_t_digest()

Delete the T-Digest

Delete the T-Digest by ID

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.DynamicResultsTDigestsApi()

try:
    # Delete the T-Digest
    api_instance.delete_t_digest()
except ApiException as e:
    print("Exception when calling DynamicResultsTDigestsApi->delete_t_digest: %s\n" % e)
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

