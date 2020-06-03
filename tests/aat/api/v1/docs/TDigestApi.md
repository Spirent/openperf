# client.TDigestApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**create_t_digest**](TDigestApi.md#create_t_digest) | **POST** /dynamic-results/t-digests | Create T-Digest
[**get_t_digest**](TDigestApi.md#get_t_digest) | **GET** /dynamic-results/t-digests/{id} | Returns the T-Digest
[**get_t_digest_result**](TDigestApi.md#get_t_digest_result) | **GET** /dynamic-results/t-digests/{id}/result | Returns the T-Digest result
[**t_digest_list**](TDigestApi.md#t_digest_list) | **GET** /dynamic-results/t-digests | List T-Digests


# **create_t_digest**
> TDigestConfig create_t_digest()

Create T-Digest

Create a new T-Digest

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TDigestApi()

try:
    # Create T-Digest
    api_response = api_instance.create_t_digest()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TDigestApi->create_t_digest: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**TDigestConfig**](TDigestConfig.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_t_digest**
> TDigestConfig get_t_digest()

Returns the T-Digest

Returns the T-Digest configuration by ID

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TDigestApi()

try:
    # Returns the T-Digest
    api_response = api_instance.get_t_digest()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TDigestApi->get_t_digest: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**TDigestConfig**](TDigestConfig.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_t_digest_result**
> TDigestResult get_t_digest_result()

Returns the T-Digest result

Returns the T-Digest result by T-Digest ID

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TDigestApi()

try:
    # Returns the T-Digest result
    api_response = api_instance.get_t_digest_result()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TDigestApi->get_t_digest_result: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**TDigestResult**](TDigestResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **t_digest_list**
> list[TDigestConfig] t_digest_list()

List T-Digests

The endpoint returns all of configured T-Digests

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TDigestApi()

try:
    # List T-Digests
    api_response = api_instance.t_digest_list()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TDigestApi->t_digest_list: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[TDigestConfig]**](TDigestConfig.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

