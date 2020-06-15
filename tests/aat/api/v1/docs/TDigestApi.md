# client.TDigestApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**create_t_digest**](TDigestApi.md#create_t_digest) | **POST** /tdigests | Create T-Digest
[**delete_t_digest**](TDigestApi.md#delete_t_digest) | **DELETE** /tdigests/{id} | Delete the T-Digest
[**delete_t_digest_result**](TDigestApi.md#delete_t_digest_result) | **DELETE** /tdigest-results/{id} | Delete the T-Digest result
[**get_t_digest**](TDigestApi.md#get_t_digest) | **GET** /tdigests/{id} | Returns the T-Digest
[**get_t_digest_result**](TDigestApi.md#get_t_digest_result) | **GET** /tdigest-results/{id} | Return the T-Digest result
[**list_t_digest_results**](TDigestApi.md#list_t_digest_results) | **GET** /tdigest-results | List T-Digest results
[**list_t_digests**](TDigestApi.md#list_t_digests) | **GET** /tdigests | List T-Digests


# **create_t_digest**
> TDigestConfig create_t_digest(tdigest)

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
tdigest = client.TDigestConfig() # TDigestConfig | T-Digest configuration

try:
    # Create T-Digest
    api_response = api_instance.create_t_digest(tdigest)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TDigestApi->create_t_digest: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **tdigest** | [**TDigestConfig**](TDigestConfig.md)| T-Digest configuration | 

### Return type

[**TDigestConfig**](TDigestConfig.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **delete_t_digest**
> delete_t_digest(id)

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
api_instance = client.TDigestApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete the T-Digest
    api_instance.delete_t_digest(id)
except ApiException as e:
    print("Exception when calling TDigestApi->delete_t_digest: %s\n" % e)
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

# **delete_t_digest_result**
> delete_t_digest_result(id)

Delete the T-Digest result

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TDigestApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete the T-Digest result
    api_instance.delete_t_digest_result(id)
except ApiException as e:
    print("Exception when calling TDigestApi->delete_t_digest_result: %s\n" % e)
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

# **get_t_digest**
> TDigestConfig get_t_digest(id)

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
id = 'id_example' # str | Unique resource identifier

try:
    # Returns the T-Digest
    api_response = api_instance.get_t_digest(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TDigestApi->get_t_digest: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**TDigestConfig**](TDigestConfig.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_t_digest_result**
> TDigestResult get_t_digest_result(id)

Return the T-Digest result

Return the T-Digest result by T-Digest ID

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TDigestApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Return the T-Digest result
    api_response = api_instance.get_t_digest_result(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TDigestApi->get_t_digest_result: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**TDigestResult**](TDigestResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_t_digest_results**
> list[TDigestResult] list_t_digest_results()

List T-Digest results

Return the list of T-Digest results

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
    # List T-Digest results
    api_response = api_instance.list_t_digest_results()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TDigestApi->list_t_digest_results: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[TDigestResult]**](TDigestResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_t_digests**
> list[TDigestConfig] list_t_digests()

List T-Digests

Return all of configured T-Digests

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
    api_response = api_instance.list_t_digests()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TDigestApi->list_t_digests: %s\n" % e)
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

