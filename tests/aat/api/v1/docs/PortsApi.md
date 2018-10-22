# client.PortsApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**create_port**](PortsApi.md#create_port) | **POST** /ports | Create a port
[**delete_port**](PortsApi.md#delete_port) | **DELETE** /ports/{id} | Delete a port
[**get_port**](PortsApi.md#get_port) | **GET** /ports/{id} | Get a port
[**list_ports**](PortsApi.md#list_ports) | **GET** /ports | List ports
[**update_port**](PortsApi.md#update_port) | **PUT** /ports/{id} | Update a port


# **create_port**
> Port create_port(port)

Create a port

Create a new port-equivalent, e.g. a bonded port.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PortsApi()
port = client.Port() # Port | New port

try:
    # Create a port
    api_response = api_instance.create_port(port)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PortsApi->create_port: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **port** | [**Port**](Port.md)| New port | 

### Return type

[**Port**](Port.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **delete_port**
> delete_port(id)

Delete a port

Deletes an existing port equivalent, e.g. a bonded port. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PortsApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete a port
    api_instance.delete_port(id)
except ApiException as e:
    print("Exception when calling PortsApi->delete_port: %s\n" % e)
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

# **get_port**
> Port get_port(id)

Get a port

Returns a port, by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PortsApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a port
    api_response = api_instance.get_port(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PortsApi->get_port: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**Port**](Port.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_ports**
> list[Port] list_ports(kind=kind)

List ports

The `ports` endpoint returns all physical ports and port-equivalents that are available for network interfaces. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PortsApi()
kind = 'kind_example' # str | Filter by kind (optional)

try:
    # List ports
    api_response = api_instance.list_ports(kind=kind)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PortsApi->list_ports: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **kind** | **str**| Filter by kind | [optional] 

### Return type

[**list[Port]**](Port.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **update_port**
> Port update_port(id, port)

Update a port

Updates an existing port's configuration. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PortsApi()
id = 'id_example' # str | Unique resource identifier
port = client.Port() # Port | Updated port

try:
    # Update a port
    api_response = api_instance.update_port(id, port)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PortsApi->update_port: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 
 **port** | [**Port**](Port.md)| Updated port | 

### Return type

[**Port**](Port.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

