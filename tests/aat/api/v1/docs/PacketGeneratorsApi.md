# client.PacketGeneratorsApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**bulk_create_generators**](PacketGeneratorsApi.md#bulk_create_generators) | **POST** /packet/generators/x/bulk-create | Bulk create packet generators
[**bulk_delete_generators**](PacketGeneratorsApi.md#bulk_delete_generators) | **POST** /packet/generators/x/bulk-delete | Bulk delete packet generators
[**bulk_start_generators**](PacketGeneratorsApi.md#bulk_start_generators) | **POST** /packet/generators/x/bulk-start | Bulk start packet generators
[**bulk_stop_generators**](PacketGeneratorsApi.md#bulk_stop_generators) | **POST** /packet/generators/x/bulk-stop | Bulk stop packet generators
[**create_generator**](PacketGeneratorsApi.md#create_generator) | **POST** /packet/generators | Create a new packet generator
[**delete_generator**](PacketGeneratorsApi.md#delete_generator) | **DELETE** /packet/generators/{id} | Delete a packet generator
[**delete_generator_result**](PacketGeneratorsApi.md#delete_generator_result) | **DELETE** /packet/generator-results/{id} | Delete a packet generator result
[**delete_generator_results**](PacketGeneratorsApi.md#delete_generator_results) | **DELETE** /packet/generator-results | Delete all generator results
[**delete_generators**](PacketGeneratorsApi.md#delete_generators) | **DELETE** /packet/generators | Delete all packet generators
[**get_generator**](PacketGeneratorsApi.md#get_generator) | **GET** /packet/generators/{id} | Get a packet generator
[**get_generator_result**](PacketGeneratorsApi.md#get_generator_result) | **GET** /packet/generator-results/{id} | Get a packet generator result
[**get_tx_flow**](PacketGeneratorsApi.md#get_tx_flow) | **GET** /packet/tx-flows/{id} | Get a transmit packet flow
[**list_generator_results**](PacketGeneratorsApi.md#list_generator_results) | **GET** /packet/generator-results | List generator results
[**list_generators**](PacketGeneratorsApi.md#list_generators) | **GET** /packet/generators | List packet generators
[**list_tx_flows**](PacketGeneratorsApi.md#list_tx_flows) | **GET** /packet/tx-flows | List packet generator transmit flows
[**start_generator**](PacketGeneratorsApi.md#start_generator) | **POST** /packet/generators/{id}/start | Start generating packets
[**stop_generator**](PacketGeneratorsApi.md#stop_generator) | **POST** /packet/generators/{id}/stop | Stop generating packets.
[**toggle_generators**](PacketGeneratorsApi.md#toggle_generators) | **POST** /packet/generators/x/toggle | Replace a running generator with a stopped generator


# **bulk_create_generators**
> BulkCreateGeneratorsResponse bulk_create_generators(create)

Bulk create packet generators

Create multiple packet generators. Requests are processed in an all-or-nothing manner, i.e. a single generator creation failure causes all generator creations for this request to fail. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketGeneratorsApi()
create = client.BulkCreateGeneratorsRequest() # BulkCreateGeneratorsRequest | Bulk creation

try:
    # Bulk create packet generators
    api_response = api_instance.bulk_create_generators(create)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PacketGeneratorsApi->bulk_create_generators: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **create** | [**BulkCreateGeneratorsRequest**](BulkCreateGeneratorsRequest.md)| Bulk creation | 

### Return type

[**BulkCreateGeneratorsResponse**](BulkCreateGeneratorsResponse.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **bulk_delete_generators**
> bulk_delete_generators(delete)

Bulk delete packet generators

Delete multiple packet generators in a best-effort manner. Generators can only be deleted when inactive. Active or Non-existant generator ids do not cause errors. Idempotent. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketGeneratorsApi()
delete = client.BulkDeleteGeneratorsRequest() # BulkDeleteGeneratorsRequest | Bulk delete

try:
    # Bulk delete packet generators
    api_instance.bulk_delete_generators(delete)
except ApiException as e:
    print("Exception when calling PacketGeneratorsApi->bulk_delete_generators: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **delete** | [**BulkDeleteGeneratorsRequest**](BulkDeleteGeneratorsRequest.md)| Bulk delete | 

### Return type

void (empty response body)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **bulk_start_generators**
> BulkStartGeneratorsResponse bulk_start_generators(start)

Bulk start packet generators

Start multiple packet generators simultaneously

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketGeneratorsApi()
start = client.BulkStartGeneratorsRequest() # BulkStartGeneratorsRequest | Bulk start

try:
    # Bulk start packet generators
    api_response = api_instance.bulk_start_generators(start)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PacketGeneratorsApi->bulk_start_generators: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **start** | [**BulkStartGeneratorsRequest**](BulkStartGeneratorsRequest.md)| Bulk start | 

### Return type

[**BulkStartGeneratorsResponse**](BulkStartGeneratorsResponse.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **bulk_stop_generators**
> bulk_stop_generators(stop)

Bulk stop packet generators

Stop multiple packet generators simultaneously

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketGeneratorsApi()
stop = client.BulkStopGeneratorsRequest() # BulkStopGeneratorsRequest | Bulk stop

try:
    # Bulk stop packet generators
    api_instance.bulk_stop_generators(stop)
except ApiException as e:
    print("Exception when calling PacketGeneratorsApi->bulk_stop_generators: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **stop** | [**BulkStopGeneratorsRequest**](BulkStopGeneratorsRequest.md)| Bulk stop | 

### Return type

void (empty response body)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **create_generator**
> PacketGenerator create_generator(generator)

Create a new packet generator

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketGeneratorsApi()
generator = client.PacketGenerator() # PacketGenerator | New packet generator

try:
    # Create a new packet generator
    api_response = api_instance.create_generator(generator)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PacketGeneratorsApi->create_generator: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **generator** | [**PacketGenerator**](PacketGenerator.md)| New packet generator | 

### Return type

[**PacketGenerator**](PacketGenerator.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **delete_generator**
> delete_generator(id)

Delete a packet generator

Delete a stopped packet generator by id. Also delete all results created by this generator. Idempotent. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketGeneratorsApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete a packet generator
    api_instance.delete_generator(id)
except ApiException as e:
    print("Exception when calling PacketGeneratorsApi->delete_generator: %s\n" % e)
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

# **delete_generator_result**
> delete_generator_result(id)

Delete a packet generator result

Delete an inactive packet generator result. Also deletes all child tx-flow objects. Idempotent. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketGeneratorsApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete a packet generator result
    api_instance.delete_generator_result(id)
except ApiException as e:
    print("Exception when calling PacketGeneratorsApi->delete_generator_result: %s\n" % e)
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

# **delete_generator_results**
> delete_generator_results()

Delete all generator results

Delete all inactive generator results

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketGeneratorsApi()

try:
    # Delete all generator results
    api_instance.delete_generator_results()
except ApiException as e:
    print("Exception when calling PacketGeneratorsApi->delete_generator_results: %s\n" % e)
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

# **delete_generators**
> delete_generators()

Delete all packet generators

Delete all inactive packet generators and their results. Idempotent. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketGeneratorsApi()

try:
    # Delete all packet generators
    api_instance.delete_generators()
except ApiException as e:
    print("Exception when calling PacketGeneratorsApi->delete_generators: %s\n" % e)
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

# **get_generator**
> PacketGenerator get_generator(id)

Get a packet generator

Return a packet generator, by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketGeneratorsApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a packet generator
    api_response = api_instance.get_generator(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PacketGeneratorsApi->get_generator: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**PacketGenerator**](PacketGenerator.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_generator_result**
> PacketGeneratorResult get_generator_result(id)

Get a packet generator result

Returns results from a packet generator by result id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketGeneratorsApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a packet generator result
    api_response = api_instance.get_generator_result(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PacketGeneratorsApi->get_generator_result: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**PacketGeneratorResult**](PacketGeneratorResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_tx_flow**
> TxFlow get_tx_flow(id)

Get a transmit packet flow

Returns a transmit packet flow by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketGeneratorsApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a transmit packet flow
    api_response = api_instance.get_tx_flow(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PacketGeneratorsApi->get_tx_flow: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**TxFlow**](TxFlow.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_generator_results**
> list[PacketGeneratorResult] list_generator_results(generator_id=generator_id, target_id=target_id)

List generator results

The `generator-results` endpoint returns all generator results created by generator instances. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketGeneratorsApi()
generator_id = 'generator_id_example' # str | Filter by generator id (optional)
target_id = 'target_id_example' # str | Filter by target port or interface id (optional)

try:
    # List generator results
    api_response = api_instance.list_generator_results(generator_id=generator_id, target_id=target_id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PacketGeneratorsApi->list_generator_results: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **generator_id** | **str**| Filter by generator id | [optional] 
 **target_id** | **str**| Filter by target port or interface id | [optional] 

### Return type

[**list[PacketGeneratorResult]**](PacketGeneratorResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_generators**
> list[PacketGenerator] list_generators(target_id=target_id)

List packet generators

The `generators` endpoint returns all packet generators that are configured to transmit test traffic. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketGeneratorsApi()
target_id = 'target_id_example' # str | Filter by target id (optional)

try:
    # List packet generators
    api_response = api_instance.list_generators(target_id=target_id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PacketGeneratorsApi->list_generators: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **target_id** | **str**| Filter by target id | [optional] 

### Return type

[**list[PacketGenerator]**](PacketGenerator.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_tx_flows**
> list[TxFlow] list_tx_flows(generator_id=generator_id, target_id=target_id)

List packet generator transmit flows

The `tx-flows` endpoint returns all packet flows that are generated by packet generators. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketGeneratorsApi()
generator_id = 'generator_id_example' # str | Filter by packet generator id (optional)
target_id = 'target_id_example' # str | Filter by target port or interface id (optional)

try:
    # List packet generator transmit flows
    api_response = api_instance.list_tx_flows(generator_id=generator_id, target_id=target_id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PacketGeneratorsApi->list_tx_flows: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **generator_id** | **str**| Filter by packet generator id | [optional] 
 **target_id** | **str**| Filter by target port or interface id | [optional] 

### Return type

[**list[TxFlow]**](TxFlow.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **start_generator**
> PacketGeneratorResult start_generator(id)

Start generating packets

Used to start a non-running generator. Creates a new generator result upon success. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketGeneratorsApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Start generating packets
    api_response = api_instance.start_generator(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PacketGeneratorsApi->start_generator: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**PacketGeneratorResult**](PacketGeneratorResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **stop_generator**
> stop_generator(id)

Stop generating packets.

Use to halt a running generator. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketGeneratorsApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Stop generating packets.
    api_instance.stop_generator(id)
except ApiException as e:
    print("Exception when calling PacketGeneratorsApi->stop_generator: %s\n" % e)
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

# **toggle_generators**
> PacketGeneratorResult toggle_generators(toggle)

Replace a running generator with a stopped generator

Atomically swap a running generator with an idle generator. Upon success, the idle generator will be in the run state and the previously running generator will be stopped. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketGeneratorsApi()
toggle = client.ToggleGeneratorsRequest() # ToggleGeneratorsRequest | Generator toggle

try:
    # Replace a running generator with a stopped generator
    api_response = api_instance.toggle_generators(toggle)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PacketGeneratorsApi->toggle_generators: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **toggle** | [**ToggleGeneratorsRequest**](ToggleGeneratorsRequest.md)| Generator toggle | 

### Return type

[**PacketGeneratorResult**](PacketGeneratorResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

