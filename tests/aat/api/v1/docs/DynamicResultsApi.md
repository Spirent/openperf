# client.DynamicResultsApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**create_t_digest**](DynamicResultsApi.md#create_t_digest) | **POST** /tdigests | Create T-Digest
[**create_threshold**](DynamicResultsApi.md#create_threshold) | **POST** /thresholds | Create a threshold
[**delete_t_digest**](DynamicResultsApi.md#delete_t_digest) | **DELETE** /tdigests/{id} | Delete the T-Digest
[**delete_t_digest_result**](DynamicResultsApi.md#delete_t_digest_result) | **DELETE** /tdigest-results/{id} | Delete the T-Digest result
[**delete_threshold**](DynamicResultsApi.md#delete_threshold) | **DELETE** /thresholds/{id} | Delete the threshold
[**delete_threshold_result**](DynamicResultsApi.md#delete_threshold_result) | **DELETE** /threshold-results/{id} | Delete the threshold result
[**get_t_digest**](DynamicResultsApi.md#get_t_digest) | **GET** /tdigests/{id} | Returns the T-Digest
[**get_t_digest_result**](DynamicResultsApi.md#get_t_digest_result) | **GET** /tdigest-results/{id} | Return the T-Digest result
[**get_threshold**](DynamicResultsApi.md#get_threshold) | **GET** /thresholds/{id} | Return the threshold
[**get_threshold_result**](DynamicResultsApi.md#get_threshold_result) | **GET** /threshold-results/{id} | Get the threshold result
[**list_t_digest_results**](DynamicResultsApi.md#list_t_digest_results) | **GET** /tdigest-results | List T-Digest results
[**list_t_digests**](DynamicResultsApi.md#list_t_digests) | **GET** /tdigests | List T-Digests
[**list_threshold_results**](DynamicResultsApi.md#list_threshold_results) | **GET** /threshold-results | List threshold results
[**list_thresholds**](DynamicResultsApi.md#list_thresholds) | **GET** /thresholds | List thresholds


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
api_instance = client.DynamicResultsApi()
tdigest = client.TDigestConfig() # TDigestConfig | T-Digest configuration

try:
    # Create T-Digest
    api_response = api_instance.create_t_digest(tdigest)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->create_t_digest: %s\n" % e)
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

# **create_threshold**
> ThresholdConfig create_threshold(threshold)

Create a threshold

Create a new threshold

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.DynamicResultsApi()
threshold = client.ThresholdConfig() # ThresholdConfig | Threshold configuration

try:
    # Create a threshold
    api_response = api_instance.create_threshold(threshold)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->create_threshold: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **threshold** | [**ThresholdConfig**](ThresholdConfig.md)| Threshold configuration | 

### Return type

[**ThresholdConfig**](ThresholdConfig.md)

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
api_instance = client.DynamicResultsApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete the T-Digest
    api_instance.delete_t_digest(id)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->delete_t_digest: %s\n" % e)
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
api_instance = client.DynamicResultsApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete the T-Digest result
    api_instance.delete_t_digest_result(id)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->delete_t_digest_result: %s\n" % e)
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

# **delete_threshold**
> delete_threshold(id)

Delete the threshold

Delete the threshold by ID

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.DynamicResultsApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete the threshold
    api_instance.delete_threshold(id)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->delete_threshold: %s\n" % e)
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

# **delete_threshold_result**
> delete_threshold_result(id)

Delete the threshold result

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.DynamicResultsApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete the threshold result
    api_instance.delete_threshold_result(id)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->delete_threshold_result: %s\n" % e)
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
api_instance = client.DynamicResultsApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Returns the T-Digest
    api_response = api_instance.get_t_digest(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->get_t_digest: %s\n" % e)
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
api_instance = client.DynamicResultsApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Return the T-Digest result
    api_response = api_instance.get_t_digest_result(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->get_t_digest_result: %s\n" % e)
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

# **get_threshold**
> ThresholdConfig get_threshold(id)

Return the threshold

The endpoint returns the threshold configuration by ID

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.DynamicResultsApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Return the threshold
    api_response = api_instance.get_threshold(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->get_threshold: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**ThresholdConfig**](ThresholdConfig.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_threshold_result**
> ThresholdResult get_threshold_result(id)

Get the threshold result

Get the threshold result by threshold ID

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.DynamicResultsApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get the threshold result
    api_response = api_instance.get_threshold_result(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->get_threshold_result: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**ThresholdResult**](ThresholdResult.md)

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
api_instance = client.DynamicResultsApi()

try:
    # List T-Digest results
    api_response = api_instance.list_t_digest_results()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->list_t_digest_results: %s\n" % e)
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
api_instance = client.DynamicResultsApi()

try:
    # List T-Digests
    api_response = api_instance.list_t_digests()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->list_t_digests: %s\n" % e)
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

# **list_threshold_results**
> list[ThresholdResult] list_threshold_results()

List threshold results

Get the list of threshold results

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.DynamicResultsApi()

try:
    # List threshold results
    api_response = api_instance.list_threshold_results()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->list_threshold_results: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[ThresholdResult]**](ThresholdResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_thresholds**
> list[ThresholdConfig] list_thresholds()

List thresholds

The endpoint returns all of the configured thresholds

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.DynamicResultsApi()

try:
    # List thresholds
    api_response = api_instance.list_thresholds()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->list_thresholds: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[ThresholdConfig]**](ThresholdConfig.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

