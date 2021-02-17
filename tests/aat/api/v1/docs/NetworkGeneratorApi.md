# client.NetworkGeneratorApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**bulk_create_network_generators**](NetworkGeneratorApi.md#bulk_create_network_generators) | **POST** /network/generators/x/bulk-create | Bulk create network generators
[**bulk_create_network_servers**](NetworkGeneratorApi.md#bulk_create_network_servers) | **POST** /network/servers/x/bulk-create | Bulk create network servers
[**bulk_delete_network_generators**](NetworkGeneratorApi.md#bulk_delete_network_generators) | **POST** /network/generators/x/bulk-delete | Bulk delete network generators
[**bulk_delete_network_servers**](NetworkGeneratorApi.md#bulk_delete_network_servers) | **POST** /network/servers/x/bulk-delete | Bulk delete network servers
[**bulk_start_network_generators**](NetworkGeneratorApi.md#bulk_start_network_generators) | **POST** /network/generators/x/bulk-start | Tell multiple network generators to start
[**bulk_stop_network_generators**](NetworkGeneratorApi.md#bulk_stop_network_generators) | **POST** /network/generators/x/bulk-stop | Bulk stop network generators
[**create_network_generator**](NetworkGeneratorApi.md#create_network_generator) | **POST** /network/generators | Create a network generator
[**create_network_server**](NetworkGeneratorApi.md#create_network_server) | **POST** /network/servers | Create and run a network server
[**delete_network_generator**](NetworkGeneratorApi.md#delete_network_generator) | **DELETE** /network/generators/{id} | Delete a network generator
[**delete_network_generator_result**](NetworkGeneratorApi.md#delete_network_generator_result) | **DELETE** /network/generator-results/{id} | Delete results from a network generator. Idempotent.
[**delete_network_server**](NetworkGeneratorApi.md#delete_network_server) | **DELETE** /network/servers/{id} | Delete a network server
[**get_network_generator**](NetworkGeneratorApi.md#get_network_generator) | **GET** /network/generators/{id} | Get a network generator
[**get_network_generator_result**](NetworkGeneratorApi.md#get_network_generator_result) | **GET** /network/generator-results/{id} | Get a result from a network generator
[**get_network_server**](NetworkGeneratorApi.md#get_network_server) | **GET** /network/servers/{id} | Get a network server
[**list_network_generator_results**](NetworkGeneratorApi.md#list_network_generator_results) | **GET** /network/generator-results | List network generator results
[**list_network_generators**](NetworkGeneratorApi.md#list_network_generators) | **GET** /network/generators | List network generators
[**list_network_servers**](NetworkGeneratorApi.md#list_network_servers) | **GET** /network/servers | List network servers
[**start_network_generator**](NetworkGeneratorApi.md#start_network_generator) | **POST** /network/generators/{id}/start | Start a network generator
[**stop_network_generator**](NetworkGeneratorApi.md#stop_network_generator) | **POST** /network/generators/{id}/stop | Stop a network generator
[**toggle_network_generators**](NetworkGeneratorApi.md#toggle_network_generators) | **POST** /network/generators/x/toggle | Replace a running network generator with a stopped network generator


# **bulk_create_network_generators**
> list[NetworkGenerator] bulk_create_network_generators(create)

Bulk create network generators

Create multiple network generators. Requests are processed in an all-or-nothing manner, i.e. a single network generator creation failure causes all creations for this request to fail. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.NetworkGeneratorApi()
create = client.BulkCreateNetworkGeneratorsRequest() # BulkCreateNetworkGeneratorsRequest | Bulk creation

try:
    # Bulk create network generators
    api_response = api_instance.bulk_create_network_generators(create)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling NetworkGeneratorApi->bulk_create_network_generators: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **create** | [**BulkCreateNetworkGeneratorsRequest**](BulkCreateNetworkGeneratorsRequest.md)| Bulk creation | 

### Return type

[**list[NetworkGenerator]**](NetworkGenerator.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **bulk_create_network_servers**
> list[NetworkServer] bulk_create_network_servers(create)

Bulk create network servers

Create multiple network servers. Requests are processed in an all-or-nothing manner, i.e. a single network server creation failure causes all creations for this request to fail. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.NetworkGeneratorApi()
create = client.BulkCreateNetworkServersRequest() # BulkCreateNetworkServersRequest | Bulk creation

try:
    # Bulk create network servers
    api_response = api_instance.bulk_create_network_servers(create)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling NetworkGeneratorApi->bulk_create_network_servers: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **create** | [**BulkCreateNetworkServersRequest**](BulkCreateNetworkServersRequest.md)| Bulk creation | 

### Return type

[**list[NetworkServer]**](NetworkServer.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **bulk_delete_network_generators**
> bulk_delete_network_generators(delete)

Bulk delete network generators

Delete multiple network generators in a best-effort manner. Non-existant network generators ids do not cause errors. Idempotent. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.NetworkGeneratorApi()
delete = client.BulkDeleteNetworkGeneratorsRequest() # BulkDeleteNetworkGeneratorsRequest | Bulk delete

try:
    # Bulk delete network generators
    api_instance.bulk_delete_network_generators(delete)
except ApiException as e:
    print("Exception when calling NetworkGeneratorApi->bulk_delete_network_generators: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **delete** | [**BulkDeleteNetworkGeneratorsRequest**](BulkDeleteNetworkGeneratorsRequest.md)| Bulk delete | 

### Return type

void (empty response body)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **bulk_delete_network_servers**
> bulk_delete_network_servers(delete)

Bulk delete network servers

Delete multiple network servers in a best-effort manner. Non-existant network server ids do not cause errors. Idempotent. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.NetworkGeneratorApi()
delete = client.BulkDeleteNetworkServersRequest() # BulkDeleteNetworkServersRequest | Bulk delete

try:
    # Bulk delete network servers
    api_instance.bulk_delete_network_servers(delete)
except ApiException as e:
    print("Exception when calling NetworkGeneratorApi->bulk_delete_network_servers: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **delete** | [**BulkDeleteNetworkServersRequest**](BulkDeleteNetworkServersRequest.md)| Bulk delete | 

### Return type

void (empty response body)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **bulk_start_network_generators**
> list[NetworkGeneratorResult] bulk_start_network_generators(bulk_start)

Tell multiple network generators to start

Start multiple network generators.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.NetworkGeneratorApi()
bulk_start = client.BulkStartNetworkGeneratorsRequest() # BulkStartNetworkGeneratorsRequest | Bulk start

try:
    # Tell multiple network generators to start
    api_response = api_instance.bulk_start_network_generators(bulk_start)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling NetworkGeneratorApi->bulk_start_network_generators: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **bulk_start** | [**BulkStartNetworkGeneratorsRequest**](BulkStartNetworkGeneratorsRequest.md)| Bulk start | 

### Return type

[**list[NetworkGeneratorResult]**](NetworkGeneratorResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **bulk_stop_network_generators**
> bulk_stop_network_generators(bulk_stop)

Bulk stop network generators

Best-effort stop multiple network generators. Non-existent network generator ids do not cause errors. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.NetworkGeneratorApi()
bulk_stop = client.BulkStopNetworkGeneratorsRequest() # BulkStopNetworkGeneratorsRequest | Bulk stop

try:
    # Bulk stop network generators
    api_instance.bulk_stop_network_generators(bulk_stop)
except ApiException as e:
    print("Exception when calling NetworkGeneratorApi->bulk_stop_network_generators: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **bulk_stop** | [**BulkStopNetworkGeneratorsRequest**](BulkStopNetworkGeneratorsRequest.md)| Bulk stop | 

### Return type

void (empty response body)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **create_network_generator**
> NetworkGenerator create_network_generator(generator)

Create a network generator

Create a new network generator

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.NetworkGeneratorApi()
generator = client.NetworkGenerator() # NetworkGenerator | New network generator

try:
    # Create a network generator
    api_response = api_instance.create_network_generator(generator)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling NetworkGeneratorApi->create_network_generator: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **generator** | [**NetworkGenerator**](NetworkGenerator.md)| New network generator | 

### Return type

[**NetworkGenerator**](NetworkGenerator.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **create_network_server**
> NetworkServer create_network_server(server)

Create and run a network server

Create a new network server.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.NetworkGeneratorApi()
server = client.NetworkServer() # NetworkServer | New network server

try:
    # Create and run a network server
    api_response = api_instance.create_network_server(server)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling NetworkGeneratorApi->create_network_server: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **server** | [**NetworkServer**](NetworkServer.md)| New network server | 

### Return type

[**NetworkServer**](NetworkServer.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **delete_network_generator**
> delete_network_generator(id)

Delete a network generator

Deletes an existing network generator. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.NetworkGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete a network generator
    api_instance.delete_network_generator(id)
except ApiException as e:
    print("Exception when calling NetworkGeneratorApi->delete_network_generator: %s\n" % e)
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

# **delete_network_generator_result**
> delete_network_generator_result(id)

Delete results from a network generator. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.NetworkGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete results from a network generator. Idempotent.
    api_instance.delete_network_generator_result(id)
except ApiException as e:
    print("Exception when calling NetworkGeneratorApi->delete_network_generator_result: %s\n" % e)
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

# **delete_network_server**
> delete_network_server(id)

Delete a network server

Deletes an existing network server. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.NetworkGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete a network server
    api_instance.delete_network_server(id)
except ApiException as e:
    print("Exception when calling NetworkGeneratorApi->delete_network_server: %s\n" % e)
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

# **get_network_generator**
> NetworkGenerator get_network_generator(id)

Get a network generator

Returns a network generator, by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.NetworkGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a network generator
    api_response = api_instance.get_network_generator(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling NetworkGeneratorApi->get_network_generator: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**NetworkGenerator**](NetworkGenerator.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_network_generator_result**
> NetworkGeneratorResult get_network_generator_result(id)

Get a result from a network generator

Returns results from a network generator by result id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.NetworkGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a result from a network generator
    api_response = api_instance.get_network_generator_result(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling NetworkGeneratorApi->get_network_generator_result: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**NetworkGeneratorResult**](NetworkGeneratorResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_network_server**
> NetworkServer get_network_server(id)

Get a network server

Returns a network server, by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.NetworkGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a network server
    api_response = api_instance.get_network_server(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling NetworkGeneratorApi->get_network_server: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**NetworkServer**](NetworkServer.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_network_generator_results**
> list[NetworkGeneratorResult] list_network_generator_results()

List network generator results

The `network-generator-results` endpoint returns all of the results produced by running network generators.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.NetworkGeneratorApi()

try:
    # List network generator results
    api_response = api_instance.list_network_generator_results()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling NetworkGeneratorApi->list_network_generator_results: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[NetworkGeneratorResult]**](NetworkGeneratorResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_network_generators**
> list[NetworkGenerator] list_network_generators()

List network generators

The `network-generators` endpoint returns all of the configured network generators.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.NetworkGeneratorApi()

try:
    # List network generators
    api_response = api_instance.list_network_generators()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling NetworkGeneratorApi->list_network_generators: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[NetworkGenerator]**](NetworkGenerator.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_network_servers**
> list[NetworkServer] list_network_servers()

List network servers

Returns all network servers

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.NetworkGeneratorApi()

try:
    # List network servers
    api_response = api_instance.list_network_servers()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling NetworkGeneratorApi->list_network_servers: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[NetworkServer]**](NetworkServer.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **start_network_generator**
> NetworkGeneratorResult start_network_generator(id, dynamic_results=dynamic_results)

Start a network generator

Start an existing network generator.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.NetworkGeneratorApi()
id = 'id_example' # str | Unique resource identifier
dynamic_results = client.DynamicResultsConfig() # DynamicResultsConfig | Dynamic results configuration (optional)

try:
    # Start a network generator
    api_response = api_instance.start_network_generator(id, dynamic_results=dynamic_results)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling NetworkGeneratorApi->start_network_generator: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 
 **dynamic_results** | [**DynamicResultsConfig**](DynamicResultsConfig.md)| Dynamic results configuration | [optional] 

### Return type

[**NetworkGeneratorResult**](NetworkGeneratorResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **stop_network_generator**
> stop_network_generator(id)

Stop a network generator

Stop a running network generator. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.NetworkGeneratorApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Stop a network generator
    api_instance.stop_network_generator(id)
except ApiException as e:
    print("Exception when calling NetworkGeneratorApi->stop_network_generator: %s\n" % e)
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

# **toggle_network_generators**
> NetworkGeneratorResult toggle_network_generators(toggle)

Replace a running network generator with a stopped network generator

Swap a running network generator with an idle network generator. Upon success, the idle generator will be in the run state, the running generator will be in the stopped state and all active TCP/UDP sessions will be transferred to the newly running generator. If the original network generator had a read/write load and the new network generator does not have this type of load, these sessions will be immediately stopped.  

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.NetworkGeneratorApi()
toggle = client.ToggleNetworkGeneratorsRequest() # ToggleNetworkGeneratorsRequest | Network generator toggle

try:
    # Replace a running network generator with a stopped network generator
    api_response = api_instance.toggle_network_generators(toggle)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling NetworkGeneratorApi->toggle_network_generators: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **toggle** | [**ToggleNetworkGeneratorsRequest**](ToggleNetworkGeneratorsRequest.md)| Network generator toggle | 

### Return type

[**NetworkGeneratorResult**](NetworkGeneratorResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

