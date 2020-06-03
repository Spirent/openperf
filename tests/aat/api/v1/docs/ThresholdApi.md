# client.ThresholdApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**create_threshold**](ThresholdApi.md#create_threshold) | **POST** /dynamic-results/thresholds | Create a threshold
[**get_threshold**](ThresholdApi.md#get_threshold) | **GET** /dynamic-results/thresholds/{id} | Returns the threshold
[**get_threshold_result**](ThresholdApi.md#get_threshold_result) | **GET** /dynamic-results/thresholds/{id}/result | Get the threshold result
[**threshold_list**](ThresholdApi.md#threshold_list) | **GET** /dynamic-results/thresholds | List thresholds


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
api_instance = client.ThresholdApi()

try:
    # Create a threshold
    api_response = api_instance.create_threshold()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling ThresholdApi->create_threshold: %s\n" % e)
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
api_instance = client.ThresholdApi()

try:
    # Returns the threshold
    api_response = api_instance.get_threshold()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling ThresholdApi->get_threshold: %s\n" % e)
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
api_instance = client.ThresholdApi()

try:
    # Get the threshold result
    api_response = api_instance.get_threshold_result()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling ThresholdApi->get_threshold_result: %s\n" % e)
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
api_instance = client.ThresholdApi()

try:
    # List thresholds
    api_response = api_instance.threshold_list()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling ThresholdApi->threshold_list: %s\n" % e)
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

