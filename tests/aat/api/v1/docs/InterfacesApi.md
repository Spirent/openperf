# client.InterfacesApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**bulk_create_interfaces**](InterfacesApi.md#bulk_create_interfaces) | **POST** /interfaces/x/bulk-create | Bulk create network interfaces
[**bulk_delete_interfaces**](InterfacesApi.md#bulk_delete_interfaces) | **POST** /interfaces/x/bulk-delete | Bulk delete network interfaces
[**create_interface**](InterfacesApi.md#create_interface) | **POST** /interfaces | Create a network interface
[**delete_interface**](InterfacesApi.md#delete_interface) | **DELETE** /interfaces/{id} | Delete a network interface
[**get_interface**](InterfacesApi.md#get_interface) | **GET** /interfaces/{id} | Get a network interface
[**list_interfaces**](InterfacesApi.md#list_interfaces) | **GET** /interfaces | List network interfaces


# **bulk_create_interfaces**
> BulkCreateInterfacesResponse bulk_create_interfaces(create)

Bulk create network interfaces

Create multiple network interfaces. Requests are processed in an all-or-nothing manner, i.e. a single network interface creation failure causes all network interface creations for this request to fail. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.InterfacesApi()
create = client.BulkCreateInterfacesRequest() # BulkCreateInterfacesRequest | Bulk creation

try:
    # Bulk create network interfaces
    api_response = api_instance.bulk_create_interfaces(create)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling InterfacesApi->bulk_create_interfaces: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **create** | [**BulkCreateInterfacesRequest**](BulkCreateInterfacesRequest.md)| Bulk creation | 

### Return type

[**BulkCreateInterfacesResponse**](BulkCreateInterfacesResponse.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **bulk_delete_interfaces**
> bulk_delete_interfaces(delete)

Bulk delete network interfaces

Best-effort delete multiple network interfaces. Non-existent interface ids do not cause errors. Idempotent. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.InterfacesApi()
delete = client.BulkDeleteInterfacesRequest() # BulkDeleteInterfacesRequest | Bulk delete

try:
    # Bulk delete network interfaces
    api_instance.bulk_delete_interfaces(delete)
except ApiException as e:
    print("Exception when calling InterfacesApi->bulk_delete_interfaces: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **delete** | [**BulkDeleteInterfacesRequest**](BulkDeleteInterfacesRequest.md)| Bulk delete | 

### Return type

void (empty response body)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **create_interface**
> Interface create_interface(interface)

Create a network interface

Create a new network interface.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.InterfacesApi()
interface = client.Interface() # Interface | New network interface

try:
    # Create a network interface
    api_response = api_instance.create_interface(interface)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling InterfacesApi->create_interface: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **interface** | [**Interface**](Interface.md)| New network interface | 

### Return type

[**Interface**](Interface.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **delete_interface**
> delete_interface(id)

Delete a network interface

Deletes an existing interface. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.InterfacesApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete a network interface
    api_instance.delete_interface(id)
except ApiException as e:
    print("Exception when calling InterfacesApi->delete_interface: %s\n" % e)
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

# **get_interface**
> Interface get_interface(id)

Get a network interface

Returns a network interface, by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.InterfacesApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a network interface
    api_response = api_instance.get_interface(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling InterfacesApi->get_interface: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**Interface**](Interface.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_interfaces**
> list[Interface] list_interfaces(port_id=port_id, eth_mac_address=eth_mac_address, ipv4_address=ipv4_address, ipv6_address=ipv6_address)

List network interfaces

The `interfaces` endpoint returns all network interfaces that are available for use as stack entry/exit points. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.InterfacesApi()
port_id = 'port_id_example' # str | Filter by port id (optional)
eth_mac_address = 'eth_mac_address_example' # str | Filter by Ethernet MAC address (optional)
ipv4_address = 'ipv4_address_example' # str | Filter by IPv4 address (optional)
ipv6_address = 'ipv6_address_example' # str | Filter by IPv6 address (optional)

try:
    # List network interfaces
    api_response = api_instance.list_interfaces(port_id=port_id, eth_mac_address=eth_mac_address, ipv4_address=ipv4_address, ipv6_address=ipv6_address)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling InterfacesApi->list_interfaces: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **port_id** | **str**| Filter by port id | [optional] 
 **eth_mac_address** | **str**| Filter by Ethernet MAC address | [optional] 
 **ipv4_address** | **str**| Filter by IPv4 address | [optional] 
 **ipv6_address** | **str**| Filter by IPv6 address | [optional] 

### Return type

[**list[Interface]**](Interface.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

