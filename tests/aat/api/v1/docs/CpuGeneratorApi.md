# client.CpuGeneratorApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**bulk_create_cpu_generators**](CpuGeneratorApi.md#bulk_create_cpu_generators) | **POST** /cpu-generators/x/bulk-create | Bulk create CPU generators
[**bulk_delete_cpu_generators**](CpuGeneratorApi.md#bulk_delete_cpu_generators) | **POST** /cpu-generators/x/bulk-delete | Bulk delete CPU generators
[**bulk_start_cpu_generators**](CpuGeneratorApi.md#bulk_start_cpu_generators) | **POST** /cpu-generators/x/bulk-start | Tell multiple CPU generators to start
[**bulk_stop_cpu_generators**](CpuGeneratorApi.md#bulk_stop_cpu_generators) | **POST** /cpu-generators/x/bulk-stop | Bulk stop CPU generators
[**cpu_info**](CpuGeneratorApi.md#cpu_info) | **GET** /cpu-info | Get a CPU info
[**create_cpu_generator**](CpuGeneratorApi.md#create_cpu_generator) | **POST** /cpu-generators | Create a CPU generator
[**delete_cpu_generator**](CpuGeneratorApi.md#delete_cpu_generator) | **DELETE** /cpu-generators/{id} | Delete a CPU generator
[**delete_cpu_generator_result**](CpuGeneratorApi.md#delete_cpu_generator_result) | **DELETE** /cpu-generator-results/{id} | Delete results from a CPU generator. Idempotent.
[**get_cpu_generator**](CpuGeneratorApi.md#get_cpu_generator) | **GET** /cpu-generators/{id} | Get a CPU generator
[**get_cpu_generator_result**](CpuGeneratorApi.md#get_cpu_generator_result) | **GET** /cpu-generator-results/{id} | Get a result from a CPU generator
[**list_cpu_generator_results**](CpuGeneratorApi.md#list_cpu_generator_results) | **GET** /cpu-generator-results | List CPU generator results
[**list_cpu_generators**](CpuGeneratorApi.md#list_cpu_generators) | **GET** /cpu-generators | List CPU generators
[**start_cpu_generator**](CpuGeneratorApi.md#start_cpu_generator) | **POST** /cpu-generators/{id}/start | Start a CPU generator
[**stop_cpu_generator**](CpuGeneratorApi.md#stop_cpu_generator) | **POST** /cpu-generators/{id}/stop | Stop a CPU generator


# **bulk_create_cpu_generators**
> list[CpuGenerator] bulk_create_cpu_generators(create)

Bulk create CPU generators

Create multiple CPU generators. Requests are processed in an all-or-nothing manner, i.e. a single CPU generator creation failure causes all creations for this request to fail. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.CpuGeneratorApi()
create = client.BulkCreateCpuGeneratorsRequest() # BulkCreateCpuGeneratorsRequest | Bulk creation

try:
    # Bulk create CPU generators
    api_response = api_instance.bulk_create_cpu_generators(create)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling CpuGeneratorApi->bulk_create_cpu_generators: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **create** | [**BulkCreateCpuGeneratorsRequest**](BulkCreateCpuGeneratorsRequest.md)| Bulk creation | 

### Return type

[**list[CpuGenerator]**](CpuGenerator.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **bulk_delete_cpu_generators**
> bulk_delete_cpu_generators(delete)

Bulk delete CPU generators

Delete multiple CPU generators in a best-effort manner. Non-existant CPU generators ids do not cause errors. Idempotent. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.CpuGeneratorApi()
delete = client.BulkDeleteCpuGeneratorsRequest() # BulkDeleteCpuGeneratorsRequest | Bulk delete

try:
    # Bulk delete CPU generators
    api_instance.bulk_delete_cpu_generators(delete)
except ApiException as e:
    print("Exception when calling CpuGeneratorApi->bulk_delete_cpu_generators: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **delete** | [**BulkDeleteCpuGeneratorsRequest**](BulkDeleteCpuGeneratorsRequest.md)| Bulk delete | 

### Return type

void (empty response body)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **bulk_start_cpu_generators**
> list[CpuGeneratorResult] bulk_start_cpu_generators(bulk_start)

Tell multiple CPU generators to start

Start multiple CPU generators.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.CpuGeneratorApi()
bulk_start = client.BulkStartCpuGeneratorsRequest() # BulkStartCpuGeneratorsRequest | Bulk start

try:
    # Tell multiple CPU generators to start
    api_response = api_instance.bulk_start_cpu_generators(bulk_start)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling CpuGeneratorApi->bulk_start_cpu_generators: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **bulk_start** | [**BulkStartCpuGeneratorsRequest**](BulkStartCpuGeneratorsRequest.md)| Bulk start | 

### Return type

[**list[CpuGeneratorResult]**](CpuGeneratorResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **bulk_stop_cpu_generators**
> bulk_stop_cpu_generators(bulk_stop)

Bulk stop CPU generators

Best-effort stop multiple CPU generators. Non-existent CPU generator ids do not cause errors. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.CpuGeneratorApi()
bulk_stop = client.BulkStopCpuGeneratorsRequest() # BulkStopCpuGeneratorsRequest | Bulk stop

try:
    # Bulk stop CPU generators
    api_instance.bulk_stop_cpu_generators(bulk_stop)
except ApiException as e:
    print("Exception when calling CpuGeneratorApi->bulk_stop_cpu_generators: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **bulk_stop** | [**BulkStopCpuGeneratorsRequest**](BulkStopCpuGeneratorsRequest.md)| Bulk stop | 

### Return type

void (empty response body)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **cpu_info**
> CpuInfoResult cpu_info()

Get a CPU info

The `cpu-info` endpoint returns CPU system info

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.CpuGeneratorApi()

try:
    # Get a CPU info
    api_response = api_instance.cpu_info()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling CpuGeneratorApi->cpu_info: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**CpuInfoResult**](CpuInfoResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **create_cpu_generator**
> CpuGenerator create_cpu_generator(generator)

Create a CPU generator

Create a new CPU generator

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.CpuGeneratorApi()
generator = client.CpuGenerator() # CpuGenerator | New CPU generator

try:
    # Create a CPU generator
    api_response = api_instance.create_cpu_generator(generator)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling CpuGeneratorApi->create_cpu_generator: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **generator** | [**CpuGenerator**](CpuGenerator.md)| New CPU generator | 

### Return type

[**CpuGenerator**](CpuGenerator.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **delete_cpu_generator**
> delete_cpu_generator(id)

Delete a CPU generator

Deletes an existing CPU generator. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.CpuGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete a CPU generator
    api_instance.delete_cpu_generator(id)
except ApiException as e:
    print("Exception when calling CpuGeneratorApi->delete_cpu_generator: %s\n" % e)
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

# **delete_cpu_generator_result**
> delete_cpu_generator_result(id)

Delete results from a CPU generator. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.CpuGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete results from a CPU generator. Idempotent.
    api_instance.delete_cpu_generator_result(id)
except ApiException as e:
    print("Exception when calling CpuGeneratorApi->delete_cpu_generator_result: %s\n" % e)
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

# **get_cpu_generator**
> CpuGenerator get_cpu_generator(id)

Get a CPU generator

Returns a CPU generator, by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.CpuGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a CPU generator
    api_response = api_instance.get_cpu_generator(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling CpuGeneratorApi->get_cpu_generator: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**CpuGenerator**](CpuGenerator.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_cpu_generator_result**
> CpuGeneratorResult get_cpu_generator_result(id)

Get a result from a CPU generator

Returns results from a CPU generator by result id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.CpuGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a result from a CPU generator
    api_response = api_instance.get_cpu_generator_result(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling CpuGeneratorApi->get_cpu_generator_result: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**CpuGeneratorResult**](CpuGeneratorResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_cpu_generator_results**
> list[CpuGeneratorResult] list_cpu_generator_results()

List CPU generator results

The `cpu-generator-results` endpoint returns all of the results produced by running CPU generators.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.CpuGeneratorApi()

try:
    # List CPU generator results
    api_response = api_instance.list_cpu_generator_results()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling CpuGeneratorApi->list_cpu_generator_results: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[CpuGeneratorResult]**](CpuGeneratorResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_cpu_generators**
> list[CpuGenerator] list_cpu_generators()

List CPU generators

The `cpu-generators` endpoint returns all of the configured CPU generators.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.CpuGeneratorApi()

try:
    # List CPU generators
    api_response = api_instance.list_cpu_generators()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling CpuGeneratorApi->list_cpu_generators: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[CpuGenerator]**](CpuGenerator.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **start_cpu_generator**
> CpuGeneratorResult start_cpu_generator(id, dynamic_results=dynamic_results)

Start a CPU generator

Start an existing CPU generator.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.CpuGeneratorApi()
id = 'id_example' # str | Unique resource identifier
dynamic_results = client.DynamicResults() # DynamicResults | Dynamic results configuration (optional)

try:
    # Start a CPU generator
    api_response = api_instance.start_cpu_generator(id, dynamic_results=dynamic_results)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling CpuGeneratorApi->start_cpu_generator: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 
 **dynamic_results** | [**DynamicResults**](DynamicResults.md)| Dynamic results configuration | [optional] 

### Return type

[**CpuGeneratorResult**](CpuGeneratorResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **stop_cpu_generator**
> stop_cpu_generator(id)

Stop a CPU generator

Stop a running CPU generator. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.CpuGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Stop a CPU generator
    api_instance.stop_cpu_generator(id)
except ApiException as e:
    print("Exception when calling CpuGeneratorApi->stop_cpu_generator: %s\n" % e)
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

