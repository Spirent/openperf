# client.MemoryGeneratorApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**bulk_start_memory_generators**](MemoryGeneratorApi.md#bulk_start_memory_generators) | **POST** /memory-generators/x/bulk-start | Tell multiple memory generators to start
[**bulk_stop_memory_generators**](MemoryGeneratorApi.md#bulk_stop_memory_generators) | **POST** /memory-generators/x/bulk-stop | Bulk stop memory generators
[**create_memory_generator**](MemoryGeneratorApi.md#create_memory_generator) | **POST** /memory-generators | Create a memory generator
[**delete_memory_generator**](MemoryGeneratorApi.md#delete_memory_generator) | **DELETE** /memory-generators/{id} | Delete a memory generator
[**delete_memory_generator_result**](MemoryGeneratorApi.md#delete_memory_generator_result) | **DELETE** /memory-generator-results/{id} | Delete results from a memory generator. Idempotent.
[**get_memory_generator**](MemoryGeneratorApi.md#get_memory_generator) | **GET** /memory-generators/{id} | Get a memory generator
[**get_memory_generator_result**](MemoryGeneratorApi.md#get_memory_generator_result) | **GET** /memory-generator-results/{id} | Get a result from a memory generator
[**list_memory_generator_results**](MemoryGeneratorApi.md#list_memory_generator_results) | **GET** /memory-generator-results | List memory generator results
[**list_memory_generators**](MemoryGeneratorApi.md#list_memory_generators) | **GET** /memory-generators | List memory generators
[**memory_info**](MemoryGeneratorApi.md#memory_info) | **GET** /memory-info | Get a memory info
[**start_memory_generator**](MemoryGeneratorApi.md#start_memory_generator) | **POST** /memory-generators/{id}/start | Start a memory generator
[**stop_memory_generator**](MemoryGeneratorApi.md#stop_memory_generator) | **POST** /memory-generators/{id}/stop | Stop a memory generator


# **bulk_start_memory_generators**
> BulkStartMemoryGeneratorsResponse bulk_start_memory_generators(bulk_start)

Tell multiple memory generators to start

Start multiple memory generators.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.MemoryGeneratorApi()
bulk_start = client.BulkStartMemoryGeneratorsRequest() # BulkStartMemoryGeneratorsRequest | Bulk start

try:
    # Tell multiple memory generators to start
    api_response = api_instance.bulk_start_memory_generators(bulk_start)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling MemoryGeneratorApi->bulk_start_memory_generators: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **bulk_start** | [**BulkStartMemoryGeneratorsRequest**](BulkStartMemoryGeneratorsRequest.md)| Bulk start | 

### Return type

[**BulkStartMemoryGeneratorsResponse**](BulkStartMemoryGeneratorsResponse.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **bulk_stop_memory_generators**
> bulk_stop_memory_generators(bulk_stop)

Bulk stop memory generators

Best-effort stop multiple memory generators. Non-existent memory generator ids do not cause errors. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.MemoryGeneratorApi()
bulk_stop = client.BulkStopMemoryGeneratorsRequest() # BulkStopMemoryGeneratorsRequest | Bulk stop

try:
    # Bulk stop memory generators
    api_instance.bulk_stop_memory_generators(bulk_stop)
except ApiException as e:
    print("Exception when calling MemoryGeneratorApi->bulk_stop_memory_generators: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **bulk_stop** | [**BulkStopMemoryGeneratorsRequest**](BulkStopMemoryGeneratorsRequest.md)| Bulk stop | 

### Return type

void (empty response body)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **create_memory_generator**
> MemoryGenerator create_memory_generator(generator)

Create a memory generator

Create a new memory generator

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.MemoryGeneratorApi()
generator = client.MemoryGenerator() # MemoryGenerator | New memory generator

try:
    # Create a memory generator
    api_response = api_instance.create_memory_generator(generator)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling MemoryGeneratorApi->create_memory_generator: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **generator** | [**MemoryGenerator**](MemoryGenerator.md)| New memory generator | 

### Return type

[**MemoryGenerator**](MemoryGenerator.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **delete_memory_generator**
> delete_memory_generator(id)

Delete a memory generator

Deletes an existing memory generator. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.MemoryGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete a memory generator
    api_instance.delete_memory_generator(id)
except ApiException as e:
    print("Exception when calling MemoryGeneratorApi->delete_memory_generator: %s\n" % e)
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

# **delete_memory_generator_result**
> delete_memory_generator_result(id)

Delete results from a memory generator. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.MemoryGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete results from a memory generator. Idempotent.
    api_instance.delete_memory_generator_result(id)
except ApiException as e:
    print("Exception when calling MemoryGeneratorApi->delete_memory_generator_result: %s\n" % e)
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

# **get_memory_generator**
> MemoryGenerator get_memory_generator(id)

Get a memory generator

Returns a memory generator, by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.MemoryGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a memory generator
    api_response = api_instance.get_memory_generator(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling MemoryGeneratorApi->get_memory_generator: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**MemoryGenerator**](MemoryGenerator.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_memory_generator_result**
> MemoryGeneratorResult get_memory_generator_result(id)

Get a result from a memory generator

Returns results from a memory generator by result id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.MemoryGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a result from a memory generator
    api_response = api_instance.get_memory_generator_result(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling MemoryGeneratorApi->get_memory_generator_result: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**MemoryGeneratorResult**](MemoryGeneratorResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_memory_generator_results**
> list[MemoryGeneratorResult] list_memory_generator_results()

List memory generator results

The `memory-generator-results` endpoint returns all of the results produced by running memory generators.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.MemoryGeneratorApi()

try:
    # List memory generator results
    api_response = api_instance.list_memory_generator_results()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling MemoryGeneratorApi->list_memory_generator_results: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[MemoryGeneratorResult]**](MemoryGeneratorResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_memory_generators**
> list[MemoryGenerator] list_memory_generators()

List memory generators

The `memory-generators` endpoint returns all of the configured memory generators.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.MemoryGeneratorApi()

try:
    # List memory generators
    api_response = api_instance.list_memory_generators()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling MemoryGeneratorApi->list_memory_generators: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[MemoryGenerator]**](MemoryGenerator.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **memory_info**
> MemoryInfoResult memory_info()

Get a memory info

The `memory-info` endpoint returns memory info values such as total and free memory size

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.MemoryGeneratorApi()

try:
    # Get a memory info
    api_response = api_instance.memory_info()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling MemoryGeneratorApi->memory_info: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**MemoryInfoResult**](MemoryInfoResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **start_memory_generator**
> start_memory_generator(id)

Start a memory generator

Start an existing memory generator. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.MemoryGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Start a memory generator
    api_instance.start_memory_generator(id)
except ApiException as e:
    print("Exception when calling MemoryGeneratorApi->start_memory_generator: %s\n" % e)
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

# **stop_memory_generator**
> stop_memory_generator(id)

Stop a memory generator

Stop a running memory generator. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.MemoryGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Stop a memory generator
    api_instance.stop_memory_generator(id)
except ApiException as e:
    print("Exception when calling MemoryGeneratorApi->stop_memory_generator: %s\n" % e)
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

