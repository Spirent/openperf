# client.TVLPApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**create_tvlp_configuration**](TVLPApi.md#create_tvlp_configuration) | **POST** /tvlp | Create a TVLP configuration
[**delete_tvlp_configuration**](TVLPApi.md#delete_tvlp_configuration) | **DELETE** /tvlp/{id} | Delete a TVLP configuration
[**delete_tvlp_result**](TVLPApi.md#delete_tvlp_result) | **DELETE** /tvlp-results/{id} | Delete a TVLP result. Idempotent.
[**get_tvlp_configuration**](TVLPApi.md#get_tvlp_configuration) | **GET** /tvlp/{id} | Get a TVLP configuration
[**get_tvlp_result**](TVLPApi.md#get_tvlp_result) | **GET** /tvlp-results/{id} | Get a result from a TLVP configuration
[**list_tvlp_configurations**](TVLPApi.md#list_tvlp_configurations) | **GET** /tvlp | List TVLP configurations
[**list_tvlp_results**](TVLPApi.md#list_tvlp_results) | **GET** /tvlp-results | List TVLP results
[**start_tvlp_configuration**](TVLPApi.md#start_tvlp_configuration) | **POST** /tvlp/{id}/start | Start a TVLP configuration
[**stop_tvlp_configuration**](TVLPApi.md#stop_tvlp_configuration) | **POST** /tvlp/{id}/stop | Stop a TVLP configuration


# **create_tvlp_configuration**
> TvlpConfiguration create_tvlp_configuration(configuration)

Create a TVLP configuration

Create a new TVLP configuration

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TVLPApi()
configuration = client.TvlpConfiguration() # TvlpConfiguration | New TVLP configuration

try:
    # Create a TVLP configuration
    api_response = api_instance.create_tvlp_configuration(configuration)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TVLPApi->create_tvlp_configuration: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **configuration** | [**TvlpConfiguration**](TvlpConfiguration.md)| New TVLP configuration | 

### Return type

[**TvlpConfiguration**](TvlpConfiguration.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **delete_tvlp_configuration**
> delete_tvlp_configuration(id)

Delete a TVLP configuration

Deletes an existing TVLP configuration. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TVLPApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete a TVLP configuration
    api_instance.delete_tvlp_configuration(id)
except ApiException as e:
    print("Exception when calling TVLPApi->delete_tvlp_configuration: %s\n" % e)
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

# **delete_tvlp_result**
> delete_tvlp_result(id)

Delete a TVLP result. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TVLPApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete a TVLP result. Idempotent.
    api_instance.delete_tvlp_result(id)
except ApiException as e:
    print("Exception when calling TVLPApi->delete_tvlp_result: %s\n" % e)
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

# **get_tvlp_configuration**
> TvlpConfiguration get_tvlp_configuration(id)

Get a TVLP configuration

Returns a TVLP configuration, by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TVLPApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a TVLP configuration
    api_response = api_instance.get_tvlp_configuration(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TVLPApi->get_tvlp_configuration: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**TvlpConfiguration**](TvlpConfiguration.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_tvlp_result**
> TvlpResult get_tvlp_result(id)

Get a result from a TLVP configuration

Returns results from a TVLP configuration by result id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TVLPApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a result from a TLVP configuration
    api_response = api_instance.get_tvlp_result(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TVLPApi->get_tvlp_result: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**TvlpResult**](TvlpResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_tvlp_configurations**
> list[TvlpConfiguration] list_tvlp_configurations()

List TVLP configurations

The `tvlp` endpoint returns all of the TVLP configurations.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TVLPApi()

try:
    # List TVLP configurations
    api_response = api_instance.list_tvlp_configurations()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TVLPApi->list_tvlp_configurations: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[TvlpConfiguration]**](TvlpConfiguration.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_tvlp_results**
> list[TvlpResult] list_tvlp_results()

List TVLP results

The `tvlp-results` endpoint returns all of the results produced by running TVLP configurations.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TVLPApi()

try:
    # List TVLP results
    api_response = api_instance.list_tvlp_results()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TVLPApi->list_tvlp_results: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[TvlpResult]**](TvlpResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **start_tvlp_configuration**
> TvlpResult start_tvlp_configuration(id, start=start)

Start a TVLP configuration

Start an existing TVLP configuration.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TVLPApi()
id = 'id_example' # str | Unique resource identifier
start = client.TvlpStartConfiguration() # TvlpStartConfiguration | TVLP Start parameters (optional)

try:
    # Start a TVLP configuration
    api_response = api_instance.start_tvlp_configuration(id, start=start)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TVLPApi->start_tvlp_configuration: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 
 **start** | [**TvlpStartConfiguration**](TvlpStartConfiguration.md)| TVLP Start parameters | [optional] 

### Return type

[**TvlpResult**](TvlpResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **stop_tvlp_configuration**
> stop_tvlp_configuration(id)

Stop a TVLP configuration

Stop a running TVLP configuration. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TVLPApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Stop a TVLP configuration
    api_instance.stop_tvlp_configuration(id)
except ApiException as e:
    print("Exception when calling TVLPApi->stop_tvlp_configuration: %s\n" % e)
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

