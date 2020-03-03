# client.AnalyzersApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**bulk_create_analyzers**](AnalyzersApi.md#bulk_create_analyzers) | **POST** /packet/analyzers/x/bulk-create | Bulk create packet analyzers
[**bulk_delete_analyzers**](AnalyzersApi.md#bulk_delete_analyzers) | **POST** /packet/analyzers/x/bulk-delete | Bulk delete packet analyzers
[**bulk_start_analyzers**](AnalyzersApi.md#bulk_start_analyzers) | **POST** /packet/analyzers/x/bulk-start | Bulk start packet analyzers
[**bulk_stop_analyzers**](AnalyzersApi.md#bulk_stop_analyzers) | **POST** /packet/analyzers/x/bulk-stop | Bulk stop packet analyzers
[**create_analyzer**](AnalyzersApi.md#create_analyzer) | **POST** /packet/analyzers | Create a packet analyzer
[**delete_analyzer**](AnalyzersApi.md#delete_analyzer) | **DELETE** /packet/analyzers/{id} | Delete a packet analyzer
[**delete_analyzer_result**](AnalyzersApi.md#delete_analyzer_result) | **DELETE** /packet/analyzer-results/{id} | Delete a packet analyzer result
[**delete_analyzer_results**](AnalyzersApi.md#delete_analyzer_results) | **DELETE** /packet/analyzer-results | Delete all analyzer results
[**delete_analyzers**](AnalyzersApi.md#delete_analyzers) | **DELETE** /packet/analyzers | Delete all packet analyzers
[**get_analyzer**](AnalyzersApi.md#get_analyzer) | **GET** /packet/analyzers/{id} | Get a packet analyzer
[**get_analyzer_result**](AnalyzersApi.md#get_analyzer_result) | **GET** /packet/analyzer-results/{id} | Get a packet analyzer result
[**get_rx_flow**](AnalyzersApi.md#get_rx_flow) | **GET** /packet/rx-flows/{id} | Get packet flow counters for a single flow
[**list_analyzer_results**](AnalyzersApi.md#list_analyzer_results) | **GET** /packet/analyzer-results | List analyzer results
[**list_analyzers**](AnalyzersApi.md#list_analyzers) | **GET** /packet/analyzers | List packet analyzers
[**list_rx_flows**](AnalyzersApi.md#list_rx_flows) | **GET** /packet/rx-flows | List received packet flows
[**start_analyzer**](AnalyzersApi.md#start_analyzer) | **POST** /packet/analyzers/{id}/start | Start analyzing and collecting packet statistics.
[**stop_analyzer**](AnalyzersApi.md#stop_analyzer) | **POST** /packet/analyzers/{id}/stop | Stop analyzing and collecting packet statistics


# **bulk_create_analyzers**
> BulkCreateAnalyzersResponse bulk_create_analyzers(create)

Bulk create packet analyzers

Create multiple packet analyzers. Requests are processed in an all-or-nothing manner, i.e. a single analyzer creation failure causes all analyzer creations for this request to fail. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.AnalyzersApi()
create = client.BulkCreateAnalyzersRequest() # BulkCreateAnalyzersRequest | Bulk creation

try:
    # Bulk create packet analyzers
    api_response = api_instance.bulk_create_analyzers(create)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling AnalyzersApi->bulk_create_analyzers: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **create** | [**BulkCreateAnalyzersRequest**](BulkCreateAnalyzersRequest.md)| Bulk creation | 

### Return type

[**BulkCreateAnalyzersResponse**](BulkCreateAnalyzersResponse.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **bulk_delete_analyzers**
> bulk_delete_analyzers(delete)

Bulk delete packet analyzers

Delete multiple packet analyzers in a best-effort manner. Analyzers can only be deleted when inactive. Active or Non-existant analyzer ids do not cause errors. Idempotent. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.AnalyzersApi()
delete = client.BulkDeleteAnalyzersRequest() # BulkDeleteAnalyzersRequest | Bulk delete

try:
    # Bulk delete packet analyzers
    api_instance.bulk_delete_analyzers(delete)
except ApiException as e:
    print("Exception when calling AnalyzersApi->bulk_delete_analyzers: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **delete** | [**BulkDeleteAnalyzersRequest**](BulkDeleteAnalyzersRequest.md)| Bulk delete | 

### Return type

void (empty response body)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **bulk_start_analyzers**
> BulkStartAnalyzersResponse bulk_start_analyzers(start)

Bulk start packet analyzers

Start multiple packet analyzers simultaneously

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.AnalyzersApi()
start = client.BulkStartAnalyzersRequest() # BulkStartAnalyzersRequest | Bulk start

try:
    # Bulk start packet analyzers
    api_response = api_instance.bulk_start_analyzers(start)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling AnalyzersApi->bulk_start_analyzers: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **start** | [**BulkStartAnalyzersRequest**](BulkStartAnalyzersRequest.md)| Bulk start | 

### Return type

[**BulkStartAnalyzersResponse**](BulkStartAnalyzersResponse.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **bulk_stop_analyzers**
> bulk_stop_analyzers(stop)

Bulk stop packet analyzers

Stop multiple packet analyzers simultaneously

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.AnalyzersApi()
stop = client.BulkStopAnalyzersRequest() # BulkStopAnalyzersRequest | Bulk stop

try:
    # Bulk stop packet analyzers
    api_instance.bulk_stop_analyzers(stop)
except ApiException as e:
    print("Exception when calling AnalyzersApi->bulk_stop_analyzers: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **stop** | [**BulkStopAnalyzersRequest**](BulkStopAnalyzersRequest.md)| Bulk stop | 

### Return type

void (empty response body)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **create_analyzer**
> Analyzer create_analyzer(analyzer)

Create a packet analyzer

Create a new packet analyzer.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.AnalyzersApi()
analyzer = client.Analyzer() # Analyzer | New packet analyzer

try:
    # Create a packet analyzer
    api_response = api_instance.create_analyzer(analyzer)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling AnalyzersApi->create_analyzer: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **analyzer** | [**Analyzer**](Analyzer.md)| New packet analyzer | 

### Return type

[**Analyzer**](Analyzer.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **delete_analyzer**
> delete_analyzer(id)

Delete a packet analyzer

Delete a stopped packet analyzer by id. Also delete all results created by this analyzer. Idempotent. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.AnalyzersApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete a packet analyzer
    api_instance.delete_analyzer(id)
except ApiException as e:
    print("Exception when calling AnalyzersApi->delete_analyzer: %s\n" % e)
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

# **delete_analyzer_result**
> delete_analyzer_result(id)

Delete a packet analyzer result

Delete an inactive packet analyzer result. Also deletes all child rx-flow objects. Idempotent. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.AnalyzersApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete a packet analyzer result
    api_instance.delete_analyzer_result(id)
except ApiException as e:
    print("Exception when calling AnalyzersApi->delete_analyzer_result: %s\n" % e)
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

# **delete_analyzer_results**
> delete_analyzer_results()

Delete all analyzer results

Delete all inactive packet analyzer results

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.AnalyzersApi()

try:
    # Delete all analyzer results
    api_instance.delete_analyzer_results()
except ApiException as e:
    print("Exception when calling AnalyzersApi->delete_analyzer_results: %s\n" % e)
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

# **delete_analyzers**
> delete_analyzers()

Delete all packet analyzers

Delete all inactive packet analyzers and their results. Idempotent. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.AnalyzersApi()

try:
    # Delete all packet analyzers
    api_instance.delete_analyzers()
except ApiException as e:
    print("Exception when calling AnalyzersApi->delete_analyzers: %s\n" % e)
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

# **get_analyzer**
> Analyzer get_analyzer(id)

Get a packet analyzer

Return a packet analyzer by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.AnalyzersApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a packet analyzer
    api_response = api_instance.get_analyzer(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling AnalyzersApi->get_analyzer: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**Analyzer**](Analyzer.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_analyzer_result**
> AnalyzerResult get_analyzer_result(id)

Get a packet analyzer result

Returns results from a packet analyzer by result id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.AnalyzersApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a packet analyzer result
    api_response = api_instance.get_analyzer_result(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling AnalyzersApi->get_analyzer_result: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**AnalyzerResult**](AnalyzerResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_rx_flow**
> RxFlow get_rx_flow(id)

Get packet flow counters for a single flow

Returns packet flow counters by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.AnalyzersApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get packet flow counters for a single flow
    api_response = api_instance.get_rx_flow(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling AnalyzersApi->get_rx_flow: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**RxFlow**](RxFlow.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_analyzer_results**
> list[AnalyzerResult] list_analyzer_results(analyzer_id=analyzer_id, source_id=source_id)

List analyzer results

The `analyzer-results` endpoint returns all analyzer results created by analyzer instances. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.AnalyzersApi()
analyzer_id = 'analyzer_id_example' # str | Filter by analyzer id (optional)
source_id = 'source_id_example' # str | Filter by receive port or interface id (optional)

try:
    # List analyzer results
    api_response = api_instance.list_analyzer_results(analyzer_id=analyzer_id, source_id=source_id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling AnalyzersApi->list_analyzer_results: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **analyzer_id** | **str**| Filter by analyzer id | [optional] 
 **source_id** | **str**| Filter by receive port or interface id | [optional] 

### Return type

[**list[AnalyzerResult]**](AnalyzerResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_analyzers**
> list[Analyzer] list_analyzers(source_id=source_id)

List packet analyzers

The `analyzers` endpoint returns all packet analyzers that are configured to collect and report port and flow statistics. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.AnalyzersApi()
source_id = 'source_id_example' # str | Filter by source id (optional)

try:
    # List packet analyzers
    api_response = api_instance.list_analyzers(source_id=source_id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling AnalyzersApi->list_analyzers: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **source_id** | **str**| Filter by source id | [optional] 

### Return type

[**list[Analyzer]**](Analyzer.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_rx_flows**
> list[RxFlow] list_rx_flows(analyzer_id=analyzer_id, source_id=source_id)

List received packet flows

The `rx-flows` endpoint returns all packet flows that have been received by analyzer instances. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.AnalyzersApi()
analyzer_id = 'analyzer_id_example' # str | Filter by receive analyzer id (optional)
source_id = 'source_id_example' # str | Filter by receive port or interface id (optional)

try:
    # List received packet flows
    api_response = api_instance.list_rx_flows(analyzer_id=analyzer_id, source_id=source_id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling AnalyzersApi->list_rx_flows: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **analyzer_id** | **str**| Filter by receive analyzer id | [optional] 
 **source_id** | **str**| Filter by receive port or interface id | [optional] 

### Return type

[**list[RxFlow]**](RxFlow.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **start_analyzer**
> AnalyzerResult start_analyzer(id)

Start analyzing and collecting packet statistics.

Used to start a non-running analyzer. Creates a new analyzer result on success. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.AnalyzersApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Start analyzing and collecting packet statistics.
    api_response = api_instance.start_analyzer(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling AnalyzersApi->start_analyzer: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**AnalyzerResult**](AnalyzerResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **stop_analyzer**
> stop_analyzer(id)

Stop analyzing and collecting packet statistics

Use to halt a running analyzer. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.AnalyzersApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Stop analyzing and collecting packet statistics
    api_instance.stop_analyzer(id)
except ApiException as e:
    print("Exception when calling AnalyzersApi->stop_analyzer: %s\n" % e)
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

