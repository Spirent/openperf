# client.LearningApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**get_packet_generator_learning_results**](LearningApi.md#get_packet_generator_learning_results) | **GET** /packet/generators/{id}/learning | Get detailed learning information
[**retry_packet_generator_learning**](LearningApi.md#retry_packet_generator_learning) | **POST** /packet/generators/{id}/learning/retry | Retry MAC learning
[**start_packet_generator_learning**](LearningApi.md#start_packet_generator_learning) | **POST** /packet/generators/{id}/learning/start | Start MAC learning
[**stop_packet_generator_learning**](LearningApi.md#stop_packet_generator_learning) | **POST** /packet/generators/{id}/learning/stop | Stop MAC learning


# **get_packet_generator_learning_results**
> PacketGeneratorLearningResults get_packet_generator_learning_results(id)

Get detailed learning information

Returns learning state and resolved addresses for a packet generator attached to an interface, by id. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.LearningApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get detailed learning information
    api_response = api_instance.get_packet_generator_learning_results(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling LearningApi->get_packet_generator_learning_results: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**PacketGeneratorLearningResults**](PacketGeneratorLearningResults.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **retry_packet_generator_learning**
> retry_packet_generator_learning(id)

Retry MAC learning

Used to retry MAC learning on a generator bound to an interface. Performs MAC learning for only unresolved addresses. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.LearningApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Retry MAC learning
    api_instance.retry_packet_generator_learning(id)
except ApiException as e:
    print("Exception when calling LearningApi->retry_packet_generator_learning: %s\n" % e)
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

# **start_packet_generator_learning**
> start_packet_generator_learning(id)

Start MAC learning

Used to start MAC learning on a generator bound to an interface. Clears previously resolved MAC table. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.LearningApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Start MAC learning
    api_instance.start_packet_generator_learning(id)
except ApiException as e:
    print("Exception when calling LearningApi->start_packet_generator_learning: %s\n" % e)
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

# **stop_packet_generator_learning**
> stop_packet_generator_learning(id)

Stop MAC learning

Used to stop MAC learning on a generator bound to an interface. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.LearningApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Stop MAC learning
    api_instance.stop_packet_generator_learning(id)
except ApiException as e:
    print("Exception when calling LearningApi->stop_packet_generator_learning: %s\n" % e)
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

