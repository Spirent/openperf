# client.BlockGeneratorApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**bulk_start_block_generators**](BlockGeneratorApi.md#bulk_start_block_generators) | **POST** /block-generators/x/bulk-start | Tell multiple block generators to start
[**bulk_stop_block_generators**](BlockGeneratorApi.md#bulk_stop_block_generators) | **POST** /block-generators/x/bulk-stop | Bulk stop block generators
[**create_block_file**](BlockGeneratorApi.md#create_block_file) | **POST** /block-files | Create a block file
[**create_block_generator**](BlockGeneratorApi.md#create_block_generator) | **POST** /block-generators | Create a block generator
[**delete_block_file**](BlockGeneratorApi.md#delete_block_file) | **DELETE** /block-files/{id} | Delete a block file
[**delete_block_generator**](BlockGeneratorApi.md#delete_block_generator) | **DELETE** /block-generators/{id} | Delete a block generator
[**delete_block_generator_result**](BlockGeneratorApi.md#delete_block_generator_result) | **DELETE** /block-generator-results/{id} | Delete results from a block generator. Idempotent.
[**get_block_device**](BlockGeneratorApi.md#get_block_device) | **GET** /block-devices/{id} | Get a block device
[**get_block_file**](BlockGeneratorApi.md#get_block_file) | **GET** /block-files/{id} | Get a block file
[**get_block_generator**](BlockGeneratorApi.md#get_block_generator) | **GET** /block-generators/{id} | Get a block generator
[**get_block_generator_result**](BlockGeneratorApi.md#get_block_generator_result) | **GET** /block-generator-results/{id} | Get a result from a block generator
[**list_block_devices**](BlockGeneratorApi.md#list_block_devices) | **GET** /block-devices | List block devices
[**list_block_files**](BlockGeneratorApi.md#list_block_files) | **GET** /block-files | List block files
[**list_block_generator_results**](BlockGeneratorApi.md#list_block_generator_results) | **GET** /block-generator-results | List block generator results
[**list_generators**](BlockGeneratorApi.md#list_generators) | **GET** /block-generators | List block generators
[**start_block_generator**](BlockGeneratorApi.md#start_block_generator) | **POST** /block-generators/{id}/start | Start a block generator
[**stop_block_generator**](BlockGeneratorApi.md#stop_block_generator) | **POST** /block-generators/{id}/stop | Stop a block generator


# **bulk_start_block_generators**
> list[BlockGeneratorResult] bulk_start_block_generators(bulk_start)

Tell multiple block generators to start

Start multiple block generators.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.BlockGeneratorApi()
bulk_start = client.BulkStartBlockGeneratorsRequest() # BulkStartBlockGeneratorsRequest | Bulk start

try:
    # Tell multiple block generators to start
    api_response = api_instance.bulk_start_block_generators(bulk_start)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling BlockGeneratorApi->bulk_start_block_generators: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **bulk_start** | [**BulkStartBlockGeneratorsRequest**](BulkStartBlockGeneratorsRequest.md)| Bulk start | 

### Return type

[**list[BlockGeneratorResult]**](BlockGeneratorResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **bulk_stop_block_generators**
> bulk_stop_block_generators(bulk_stop)

Bulk stop block generators

Best-effort stop multiple block generators. Non-existent block generator ids do not cause errors. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.BlockGeneratorApi()
bulk_stop = client.BulkStopBlockGeneratorsRequest() # BulkStopBlockGeneratorsRequest | Bulk stop

try:
    # Bulk stop block generators
    api_instance.bulk_stop_block_generators(bulk_stop)
except ApiException as e:
    print("Exception when calling BlockGeneratorApi->bulk_stop_block_generators: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **bulk_stop** | [**BulkStopBlockGeneratorsRequest**](BulkStopBlockGeneratorsRequest.md)| Bulk stop | 

### Return type

void (empty response body)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **create_block_file**
> BlockFile create_block_file(file)

Create a block file

Create a new block file.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.BlockGeneratorApi()
file = client.BlockFile() # BlockFile | New block file

try:
    # Create a block file
    api_response = api_instance.create_block_file(file)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling BlockGeneratorApi->create_block_file: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **file** | [**BlockFile**](BlockFile.md)| New block file | 

### Return type

[**BlockFile**](BlockFile.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **create_block_generator**
> BlockGenerator create_block_generator(generator)

Create a block generator

Create a new block generator

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.BlockGeneratorApi()
generator = client.BlockGenerator() # BlockGenerator | New block generator

try:
    # Create a block generator
    api_response = api_instance.create_block_generator(generator)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling BlockGeneratorApi->create_block_generator: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **generator** | [**BlockGenerator**](BlockGenerator.md)| New block generator | 

### Return type

[**BlockGenerator**](BlockGenerator.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **delete_block_file**
> delete_block_file(id)

Delete a block file

Deletes an existing block file. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.BlockGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete a block file
    api_instance.delete_block_file(id)
except ApiException as e:
    print("Exception when calling BlockGeneratorApi->delete_block_file: %s\n" % e)
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

# **delete_block_generator**
> delete_block_generator(id)

Delete a block generator

Deletes an existing block generator. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.BlockGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete a block generator
    api_instance.delete_block_generator(id)
except ApiException as e:
    print("Exception when calling BlockGeneratorApi->delete_block_generator: %s\n" % e)
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

# **delete_block_generator_result**
> delete_block_generator_result(id)

Delete results from a block generator. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.BlockGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete results from a block generator. Idempotent.
    api_instance.delete_block_generator_result(id)
except ApiException as e:
    print("Exception when calling BlockGeneratorApi->delete_block_generator_result: %s\n" % e)
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

# **get_block_device**
> BlockDevice get_block_device(id)

Get a block device

Returns a block device, by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.BlockGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a block device
    api_response = api_instance.get_block_device(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling BlockGeneratorApi->get_block_device: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**BlockDevice**](BlockDevice.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_block_file**
> BlockFile get_block_file(id)

Get a block file

Returns a block file, by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.BlockGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a block file
    api_response = api_instance.get_block_file(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling BlockGeneratorApi->get_block_file: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**BlockFile**](BlockFile.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_block_generator**
> BlockGenerator get_block_generator(id)

Get a block generator

Returns a block generator, by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.BlockGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a block generator
    api_response = api_instance.get_block_generator(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling BlockGeneratorApi->get_block_generator: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**BlockGenerator**](BlockGenerator.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_block_generator_result**
> BlockGeneratorResult get_block_generator_result(id)

Get a result from a block generator

Returns results from a block generator by result id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.BlockGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a result from a block generator
    api_response = api_instance.get_block_generator_result(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling BlockGeneratorApi->get_block_generator_result: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**BlockGeneratorResult**](BlockGeneratorResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_block_devices**
> list[BlockDevice] list_block_devices()

List block devices

The `block-devices` endpoint returns all block devices that are eligible as load generation targets.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.BlockGeneratorApi()

try:
    # List block devices
    api_response = api_instance.list_block_devices()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling BlockGeneratorApi->list_block_devices: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[BlockDevice]**](BlockDevice.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_block_files**
> list[BlockFile] list_block_files()

List block files

The `block-files` endpoint returns all block files that are eligible as load generation targets.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.BlockGeneratorApi()

try:
    # List block files
    api_response = api_instance.list_block_files()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling BlockGeneratorApi->list_block_files: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[BlockFile]**](BlockFile.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_block_generator_results**
> list[BlockGeneratorResult] list_block_generator_results()

List block generator results

The `block-generator-results` endpoint returns all of the results produced by running block generators.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.BlockGeneratorApi()

try:
    # List block generator results
    api_response = api_instance.list_block_generator_results()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling BlockGeneratorApi->list_block_generator_results: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[BlockGeneratorResult]**](BlockGeneratorResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_generators**
> list[BlockGenerator] list_generators()

List block generators

The `block-generators` endpoint returns all of the configured block generators.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.BlockGeneratorApi()

try:
    # List block generators
    api_response = api_instance.list_generators()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling BlockGeneratorApi->list_generators: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[BlockGenerator]**](BlockGenerator.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **start_block_generator**
> BlockGeneratorResult start_block_generator(id)

Start a block generator

Start an existing block generator.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.BlockGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Start a block generator
    api_response = api_instance.start_block_generator(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling BlockGeneratorApi->start_block_generator: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**BlockGeneratorResult**](BlockGeneratorResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **stop_block_generator**
> stop_block_generator(id)

Stop a block generator

Stop a running block generator. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.BlockGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Stop a block generator
    api_instance.stop_block_generator(id)
except ApiException as e:
    print("Exception when calling BlockGeneratorApi->stop_block_generator: %s\n" % e)
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

