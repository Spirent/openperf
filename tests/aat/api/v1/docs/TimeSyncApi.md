# client.TimeSyncApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**create_time_source**](TimeSyncApi.md#create_time_source) | **POST** /time-sources | Register a time source for time syncing.
[**delete_time_source**](TimeSyncApi.md#delete_time_source) | **DELETE** /time-sources/{id} | Delete a time source
[**get_time_counter**](TimeSyncApi.md#get_time_counter) | **GET** /time-counters/{id} | Get a time counter
[**get_time_keeper**](TimeSyncApi.md#get_time_keeper) | **GET** /time-keeper | Get a time keeper.
[**get_time_source**](TimeSyncApi.md#get_time_source) | **GET** /time-sources/{id} | Get a time source
[**list_time_counters**](TimeSyncApi.md#list_time_counters) | **GET** /time-counters | List time counters
[**list_time_sources**](TimeSyncApi.md#list_time_sources) | **GET** /time-sources | List reference clocks


# **create_time_source**
> TimeSource create_time_source(timesource)

Register a time source for time syncing.

Registers a new time source for time syncing. Time sources are immutable. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TimeSyncApi()
timesource = client.TimeSource() # TimeSource | New time source

try:
    # Register a time source for time syncing.
    api_response = api_instance.create_time_source(timesource)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TimeSyncApi->create_time_source: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **timesource** | [**TimeSource**](TimeSource.md)| New time source | 

### Return type

[**TimeSource**](TimeSource.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **delete_time_source**
> delete_time_source(id)

Delete a time source

Deletes an existing time source. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TimeSyncApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete a time source
    api_instance.delete_time_source(id)
except ApiException as e:
    print("Exception when calling TimeSyncApi->delete_time_source: %s\n" % e)
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

# **get_time_counter**
> TimeCounter get_time_counter(id)

Get a time counter

Returns a time counter, by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TimeSyncApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a time counter
    api_response = api_instance.get_time_counter(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TimeSyncApi->get_time_counter: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**TimeCounter**](TimeCounter.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_time_keeper**
> TimeKeeper get_time_keeper()

Get a time keeper.

Returns the system time keeper, aka clock.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TimeSyncApi()

try:
    # Get a time keeper.
    api_response = api_instance.get_time_keeper()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TimeSyncApi->get_time_keeper: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**TimeKeeper**](TimeKeeper.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_time_source**
> TimeSource get_time_source(id)

Get a time source

Get a time source, by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TimeSyncApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a time source
    api_response = api_instance.get_time_source(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TimeSyncApi->get_time_source: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**TimeSource**](TimeSource.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_time_counters**
> list[TimeCounter] list_time_counters()

List time counters

The `time-counters` endpoint returns all local time counters that are available for measuring the passage of time.  This list is for informational purposes only. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TimeSyncApi()

try:
    # List time counters
    api_response = api_instance.list_time_counters()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TimeSyncApi->list_time_counters: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[TimeCounter]**](TimeCounter.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_time_sources**
> list[TimeSource] list_time_sources()

List reference clocks

The `time-sources` endpoint returns all time sources that are used for syncing the local time. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.TimeSyncApi()

try:
    # List reference clocks
    api_response = api_instance.list_time_sources()
    pprint(api_response)
except ApiException as e:
    print("Exception when calling TimeSyncApi->list_time_sources: %s\n" % e)
```

### Parameters
This endpoint does not need any parameter.

### Return type

[**list[TimeSource]**](TimeSource.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

