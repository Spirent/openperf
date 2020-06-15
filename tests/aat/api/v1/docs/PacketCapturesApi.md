# client.PacketCapturesApi

All URIs are relative to *http://localhost*

Method | HTTP request | Description
------------- | ------------- | -------------
[**bulk_create_packet_captures**](PacketCapturesApi.md#bulk_create_packet_captures) | **POST** /packet/captures/x/bulk-create | Bulk create packet captures
[**bulk_delete_packet_captures**](PacketCapturesApi.md#bulk_delete_packet_captures) | **POST** /packet/captures/x/bulk-delete | Bulk delete packet captures
[**bulk_start_packet_captures**](PacketCapturesApi.md#bulk_start_packet_captures) | **POST** /packet/captures/x/bulk-start | Bulk start packet captures
[**bulk_stop_packet_captures**](PacketCapturesApi.md#bulk_stop_packet_captures) | **POST** /packet/captures/x/bulk-stop | Bulk stop packet captures
[**create_packet_capture**](PacketCapturesApi.md#create_packet_capture) | **POST** /packet/captures | Create a packet capture
[**delete_packet_capture**](PacketCapturesApi.md#delete_packet_capture) | **DELETE** /packet/captures/{id} | Delete a packet capture
[**delete_packet_capture_result**](PacketCapturesApi.md#delete_packet_capture_result) | **DELETE** /packet/capture-results/{id} | Delete a packet capture result
[**delete_packet_capture_results**](PacketCapturesApi.md#delete_packet_capture_results) | **DELETE** /packet/capture-results | Delete all capture results
[**delete_packet_captures**](PacketCapturesApi.md#delete_packet_captures) | **DELETE** /packet/captures | Delete all packet captures
[**get_packet_capture**](PacketCapturesApi.md#get_packet_capture) | **GET** /packet/captures/{id} | Get a packet capture
[**get_packet_capture_live**](PacketCapturesApi.md#get_packet_capture_live) | **GET** /packet/capture-results/{id}/live | Get live capture packet data as a pcap file
[**get_packet_capture_pcap**](PacketCapturesApi.md#get_packet_capture_pcap) | **GET** /packet/capture-results/{id}/pcap | Get a packet data as a pcap file
[**get_packet_capture_result**](PacketCapturesApi.md#get_packet_capture_result) | **GET** /packet/capture-results/{id} | Get a packet capture result
[**list_packet_capture_results**](PacketCapturesApi.md#list_packet_capture_results) | **GET** /packet/capture-results | List capture results
[**list_packet_captures**](PacketCapturesApi.md#list_packet_captures) | **GET** /packet/captures | List packet capture
[**start_packet_capture**](PacketCapturesApi.md#start_packet_capture) | **POST** /packet/captures/{id}/start | Start packet capture.
[**stop_packet_capture**](PacketCapturesApi.md#stop_packet_capture) | **POST** /packet/captures/{id}/stop | Stop packet capture.


# **bulk_create_packet_captures**
> BulkCreatePacketCapturesResponse bulk_create_packet_captures(create)

Bulk create packet captures

Create multiple packet captures. Requests are processed in an all-or-nothing manner, i.e. a single capture creation failure causes all capture creations for this request to fail. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketCapturesApi()
create = client.BulkCreatePacketCapturesRequest() # BulkCreatePacketCapturesRequest | Bulk creation

try:
    # Bulk create packet captures
    api_response = api_instance.bulk_create_packet_captures(create)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PacketCapturesApi->bulk_create_packet_captures: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **create** | [**BulkCreatePacketCapturesRequest**](BulkCreatePacketCapturesRequest.md)| Bulk creation | 

### Return type

[**BulkCreatePacketCapturesResponse**](BulkCreatePacketCapturesResponse.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **bulk_delete_packet_captures**
> bulk_delete_packet_captures(delete)

Bulk delete packet captures

Delete multiple packet captures in a best-effort manner. Captures can only be deleted when inactive. Active or Non-existant capture ids do not cause errors.  Also deletes results and captured packets associated with the capture. Idempotent. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketCapturesApi()
delete = client.BulkDeletePacketCapturesRequest() # BulkDeletePacketCapturesRequest | Bulk delete

try:
    # Bulk delete packet captures
    api_instance.bulk_delete_packet_captures(delete)
except ApiException as e:
    print("Exception when calling PacketCapturesApi->bulk_delete_packet_captures: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **delete** | [**BulkDeletePacketCapturesRequest**](BulkDeletePacketCapturesRequest.md)| Bulk delete | 

### Return type

void (empty response body)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **bulk_start_packet_captures**
> BulkStartPacketCapturesResponse bulk_start_packet_captures(start)

Bulk start packet captures

Start multiple packet captures simultaneously

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketCapturesApi()
start = client.BulkStartPacketCapturesRequest() # BulkStartPacketCapturesRequest | Bulk start

try:
    # Bulk start packet captures
    api_response = api_instance.bulk_start_packet_captures(start)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PacketCapturesApi->bulk_start_packet_captures: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **start** | [**BulkStartPacketCapturesRequest**](BulkStartPacketCapturesRequest.md)| Bulk start | 

### Return type

[**BulkStartPacketCapturesResponse**](BulkStartPacketCapturesResponse.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **bulk_stop_packet_captures**
> bulk_stop_packet_captures(stop)

Bulk stop packet captures

Stop multiple packet captures simultaneously

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketCapturesApi()
stop = client.BulkStopPacketCapturesRequest() # BulkStopPacketCapturesRequest | Bulk stop

try:
    # Bulk stop packet captures
    api_instance.bulk_stop_packet_captures(stop)
except ApiException as e:
    print("Exception when calling PacketCapturesApi->bulk_stop_packet_captures: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **stop** | [**BulkStopPacketCapturesRequest**](BulkStopPacketCapturesRequest.md)| Bulk stop | 

### Return type

void (empty response body)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **create_packet_capture**
> PacketCapture create_packet_capture(capture)

Create a packet capture

Create a new packet capture.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketCapturesApi()
capture = client.PacketCapture() # PacketCapture | New packet capture

try:
    # Create a packet capture
    api_response = api_instance.create_packet_capture(capture)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PacketCapturesApi->create_packet_capture: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **capture** | [**PacketCapture**](PacketCapture.md)| New packet capture | 

### Return type

[**PacketCapture**](PacketCapture.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **delete_packet_capture**
> delete_packet_capture(id)

Delete a packet capture

Delete a stopped packet capture by id. Also deletes results and captured packets associated with the capture. Idempotent. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketCapturesApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete a packet capture
    api_instance.delete_packet_capture(id)
except ApiException as e:
    print("Exception when calling PacketCapturesApi->delete_packet_capture: %s\n" % e)
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

# **delete_packet_capture_result**
> delete_packet_capture_result(id)

Delete a packet capture result

Delete an inactive packet capture result. Also deletes captured packets associated with the results. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketCapturesApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Delete a packet capture result
    api_instance.delete_packet_capture_result(id)
except ApiException as e:
    print("Exception when calling PacketCapturesApi->delete_packet_capture_result: %s\n" % e)
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

# **delete_packet_capture_results**
> delete_packet_capture_results()

Delete all capture results

Delete all inactive packet capture results. Also deletes captured packets associated with the results.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketCapturesApi()

try:
    # Delete all capture results
    api_instance.delete_packet_capture_results()
except ApiException as e:
    print("Exception when calling PacketCapturesApi->delete_packet_capture_results: %s\n" % e)
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

# **delete_packet_captures**
> delete_packet_captures()

Delete all packet captures

Delete all inactive packet captures. Also deletes captured packets and results associated with the capture.  Idempotent. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketCapturesApi()

try:
    # Delete all packet captures
    api_instance.delete_packet_captures()
except ApiException as e:
    print("Exception when calling PacketCapturesApi->delete_packet_captures: %s\n" % e)
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

# **get_packet_capture**
> PacketCapture get_packet_capture(id)

Get a packet capture

Return a packet capture by id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketCapturesApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a packet capture
    api_response = api_instance.get_packet_capture(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PacketCapturesApi->get_packet_capture: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**PacketCapture**](PacketCapture.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_packet_capture_live**
> file get_packet_capture_live(id)

Get live capture packet data as a pcap file

Returns a pcap file of the captured data.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketCapturesApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get live capture packet data as a pcap file
    api_response = api_instance.get_packet_capture_live(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PacketCapturesApi->get_packet_capture_live: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**file**](file.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/x-pcapng

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_packet_capture_pcap**
> file get_packet_capture_pcap(id)

Get a packet data as a pcap file

Returns a pcap file of the captured data.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketCapturesApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a packet data as a pcap file
    api_response = api_instance.get_packet_capture_pcap(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PacketCapturesApi->get_packet_capture_pcap: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**file**](file.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/x-pcapng

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **get_packet_capture_result**
> PacketCaptureResult get_packet_capture_result(id)

Get a packet capture result

Returns results from a packet capture by result id.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketCapturesApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Get a packet capture result
    api_response = api_instance.get_packet_capture_result(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PacketCapturesApi->get_packet_capture_result: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**PacketCaptureResult**](PacketCaptureResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_packet_capture_results**
> list[PacketCaptureResult] list_packet_capture_results(capture_id=capture_id, source_id=source_id)

List capture results

The `capture-results` endpoint returns all capture results created by capture instances. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketCapturesApi()
capture_id = 'capture_id_example' # str | Filter by capture id (optional)
source_id = 'source_id_example' # str | Filter by receive port or interface id (optional)

try:
    # List capture results
    api_response = api_instance.list_packet_capture_results(capture_id=capture_id, source_id=source_id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PacketCapturesApi->list_packet_capture_results: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **capture_id** | **str**| Filter by capture id | [optional] 
 **source_id** | **str**| Filter by receive port or interface id | [optional] 

### Return type

[**list[PacketCaptureResult]**](PacketCaptureResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **list_packet_captures**
> list[PacketCapture] list_packet_captures(source_id=source_id)

List packet capture

The `captures` endpoint returns all configured packet captures. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketCapturesApi()
source_id = 'source_id_example' # str | Filter by source id (optional)

try:
    # List packet capture
    api_response = api_instance.list_packet_captures(source_id=source_id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PacketCapturesApi->list_packet_captures: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **source_id** | **str**| Filter by source id | [optional] 

### Return type

[**list[PacketCapture]**](PacketCapture.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **start_packet_capture**
> PacketCaptureResult start_packet_capture(id)

Start packet capture.

Used to start a non-running capture. Creates a new capture. result on success. 

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketCapturesApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Start packet capture.
    api_response = api_instance.start_packet_capture(id)
    pprint(api_response)
except ApiException as e:
    print("Exception when calling PacketCapturesApi->start_packet_capture: %s\n" % e)
```

### Parameters

Name | Type | Description  | Notes
------------- | ------------- | ------------- | -------------
 **id** | **str**| Unique resource identifier | 

### Return type

[**PacketCaptureResult**](PacketCaptureResult.md)

### Authorization

No authorization required

### HTTP request headers

 - **Content-Type**: application/json
 - **Accept**: application/json

[[Back to top]](#) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to Model list]](../README.md#documentation-for-models) [[Back to README]](../README.md)

# **stop_packet_capture**
> stop_packet_capture(id)

Stop packet capture.

Use to halt a running capture. Idempotent.

### Example
```python
from __future__ import print_function
import time
import client
from client.rest import ApiException
from pprint import pprint

# create an instance of the API class
api_instance = client.PacketCapturesApi()
id = 'id_example' # str | Unique resource identifier

try:
    # Stop packet capture.
    api_instance.stop_packet_capture(id)
except ApiException as e:
    print("Exception when calling PacketCapturesApi->stop_packet_capture: %s\n" % e)
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

