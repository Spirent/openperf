# client.DynamicResultsApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**create_t_digest**](DynamicResultsApi.md#create_t_digest) | **POST** /dynamic-results/t-digests | Create T-Digest
[**create_threshold**](DynamicResultsApi.md#create_threshold) | **POST** /dynamic-results/thresholds | Create a threshold
[**get_t_digest**](DynamicResultsApi.md#get_t_digest) | **GET** /dynamic-results/t-digests/{id} | Returns the T-Digest
[**get_t_digest_result**](DynamicResultsApi.md#get_t_digest_result) | **GET** /dynamic-results/t-digests/{id}/result | Returns the T-Digest result
[**get_threshold**](DynamicResultsApi.md#get_threshold) | **GET** /dynamic-results/thresholds/{id} | Returns the threshold
[**get_threshold_result**](DynamicResultsApi.md#get_threshold_result) | **GET** /dynamic-results/thresholds/{id}/result | Get the threshold result
[**t_digest_list**](DynamicResultsApi.md#t_digest_list) | **GET** /dynamic-results/t-digests | List T-Digests
[**threshold_list**](DynamicResultsApi.md#threshold_list) | **GET** /dynamic-results/thresholds | List thresholds


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
api_instance = client.DynamicResultsApi()

try:
    # Create T-Digest
    api_response = api_instance.create_t_digest()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->create_t_digest: %s\n" % e)
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

# **create_threshold**
> ThresholdConfig create_threshold()

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

try:
    # Create a threshold
    api_response = api_instance.create_threshold()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->create_threshold: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**ThresholdConfig**](ThresholdConfig.md)

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
api_instance = client.DynamicResultsApi()

try:
    # Returns the T-Digest
    api_response = api_instance.get_t_digest()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->get_t_digest: %s\n" % e)
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
api_instance = client.DynamicResultsApi()

try:
    # Returns the T-Digest result
    api_response = api_instance.get_t_digest_result()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->get_t_digest_result: %s\n" % e)
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

# **get_threshold**
> ThresholdConfig get_threshold()

Returns the threshold

Returns the threshold configuration by ID

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
    # Returns the threshold
    api_response = api_instance.get_threshold()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->get_threshold: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**ThresholdConfig**](ThresholdConfig.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_threshold_result**
> ThresholdResult get_threshold_result()

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

try:
    # Get the threshold result
    api_response = api_instance.get_threshold_result()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->get_threshold_result: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**ThresholdResult**](ThresholdResult.md)

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
api_instance = client.DynamicResultsApi()

try:
    # List T-Digests
    api_response = api_instance.t_digest_list()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->t_digest_list: %s\n" % e)
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

# **threshold_list**
> list[ThresholdConfig] threshold_list()

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
    api_response = api_instance.threshold_list()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling DynamicResultsApi->threshold_list: %s\n" % e)
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

