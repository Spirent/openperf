# coding: utf-8

"""
    OpenPerf API

    REST API interface for OpenPerf  # noqa: E501

    OpenAPI spec version: 1
    Contact: support@spirent.com
    Generated by: https://github.com/swagger-api/swagger-codegen.git
"""


import pprint
import re  # noqa: F401

import six


class PacketProtocolTcp(object):
    """NOTE: This class is auto generated by the swagger code generator program.

    Do not edit the class manually.
    """

    """
    Attributes:
      swagger_types (dict): The key is attribute name
                            and the value is attribute type.
      attribute_map (dict): The key is attribute name
                            and the value is json key in definition.
    """
    swagger_types = {
        'ack': 'int',
        'checksum': 'int',
        'data_offset': 'int',
        'destination': 'int',
        'flags': 'list[str]',
        'reserved': 'int',
        'sequence': 'int',
        'source': 'int',
        'urgent_pointer': 'int',
        'window': 'int'
    }

    attribute_map = {
        'ack': 'ack',
        'checksum': 'checksum',
        'data_offset': 'data_offset',
        'destination': 'destination',
        'flags': 'flags',
        'reserved': 'reserved',
        'sequence': 'sequence',
        'source': 'source',
        'urgent_pointer': 'urgent_pointer',
        'window': 'window'
    }

    def __init__(self, ack=None, checksum=None, data_offset=None, destination=None, flags=None, reserved=None, sequence=None, source=None, urgent_pointer=None, window=None):  # noqa: E501
        """PacketProtocolTcp - a model defined in Swagger"""  # noqa: E501

        self._ack = None
        self._checksum = None
        self._data_offset = None
        self._destination = None
        self._flags = None
        self._reserved = None
        self._sequence = None
        self._source = None
        self._urgent_pointer = None
        self._window = None
        self.discriminator = None

        if ack is not None:
            self.ack = ack
        if checksum is not None:
            self.checksum = checksum
        if data_offset is not None:
            self.data_offset = data_offset
        if destination is not None:
            self.destination = destination
        if flags is not None:
            self.flags = flags
        if reserved is not None:
            self.reserved = reserved
        if sequence is not None:
            self.sequence = sequence
        if source is not None:
            self.source = source
        if urgent_pointer is not None:
            self.urgent_pointer = urgent_pointer
        if window is not None:
            self.window = window

    @property
    def ack(self):
        """Gets the ack of this PacketProtocolTcp.  # noqa: E501

        tcp ack  # noqa: E501

        :return: The ack of this PacketProtocolTcp.  # noqa: E501
        :rtype: int
        """
        return self._ack

    @ack.setter
    def ack(self, ack):
        """Sets the ack of this PacketProtocolTcp.

        tcp ack  # noqa: E501

        :param ack: The ack of this PacketProtocolTcp.  # noqa: E501
        :type: int
        """
        self._ack = ack

    @property
    def checksum(self):
        """Gets the checksum of this PacketProtocolTcp.  # noqa: E501

        tcp checksum  # noqa: E501

        :return: The checksum of this PacketProtocolTcp.  # noqa: E501
        :rtype: int
        """
        return self._checksum

    @checksum.setter
    def checksum(self, checksum):
        """Sets the checksum of this PacketProtocolTcp.

        tcp checksum  # noqa: E501

        :param checksum: The checksum of this PacketProtocolTcp.  # noqa: E501
        :type: int
        """
        self._checksum = checksum

    @property
    def data_offset(self):
        """Gets the data_offset of this PacketProtocolTcp.  # noqa: E501

        tcp data offset  # noqa: E501

        :return: The data_offset of this PacketProtocolTcp.  # noqa: E501
        :rtype: int
        """
        return self._data_offset

    @data_offset.setter
    def data_offset(self, data_offset):
        """Sets the data_offset of this PacketProtocolTcp.

        tcp data offset  # noqa: E501

        :param data_offset: The data_offset of this PacketProtocolTcp.  # noqa: E501
        :type: int
        """
        self._data_offset = data_offset

    @property
    def destination(self):
        """Gets the destination of this PacketProtocolTcp.  # noqa: E501

        tcp destination  # noqa: E501

        :return: The destination of this PacketProtocolTcp.  # noqa: E501
        :rtype: int
        """
        return self._destination

    @destination.setter
    def destination(self, destination):
        """Sets the destination of this PacketProtocolTcp.

        tcp destination  # noqa: E501

        :param destination: The destination of this PacketProtocolTcp.  # noqa: E501
        :type: int
        """
        self._destination = destination

    @property
    def flags(self):
        """Gets the flags of this PacketProtocolTcp.  # noqa: E501

        tcp flags  # noqa: E501

        :return: The flags of this PacketProtocolTcp.  # noqa: E501
        :rtype: list[str]
        """
        return self._flags

    @flags.setter
    def flags(self, flags):
        """Sets the flags of this PacketProtocolTcp.

        tcp flags  # noqa: E501

        :param flags: The flags of this PacketProtocolTcp.  # noqa: E501
        :type: list[str]
        """
        self._flags = flags

    @property
    def reserved(self):
        """Gets the reserved of this PacketProtocolTcp.  # noqa: E501

        tcp reserved  # noqa: E501

        :return: The reserved of this PacketProtocolTcp.  # noqa: E501
        :rtype: int
        """
        return self._reserved

    @reserved.setter
    def reserved(self, reserved):
        """Sets the reserved of this PacketProtocolTcp.

        tcp reserved  # noqa: E501

        :param reserved: The reserved of this PacketProtocolTcp.  # noqa: E501
        :type: int
        """
        self._reserved = reserved

    @property
    def sequence(self):
        """Gets the sequence of this PacketProtocolTcp.  # noqa: E501

        tcp sequence  # noqa: E501

        :return: The sequence of this PacketProtocolTcp.  # noqa: E501
        :rtype: int
        """
        return self._sequence

    @sequence.setter
    def sequence(self, sequence):
        """Sets the sequence of this PacketProtocolTcp.

        tcp sequence  # noqa: E501

        :param sequence: The sequence of this PacketProtocolTcp.  # noqa: E501
        :type: int
        """
        self._sequence = sequence

    @property
    def source(self):
        """Gets the source of this PacketProtocolTcp.  # noqa: E501

        tcp source  # noqa: E501

        :return: The source of this PacketProtocolTcp.  # noqa: E501
        :rtype: int
        """
        return self._source

    @source.setter
    def source(self, source):
        """Sets the source of this PacketProtocolTcp.

        tcp source  # noqa: E501

        :param source: The source of this PacketProtocolTcp.  # noqa: E501
        :type: int
        """
        self._source = source

    @property
    def urgent_pointer(self):
        """Gets the urgent_pointer of this PacketProtocolTcp.  # noqa: E501

        tcp urgent pointer  # noqa: E501

        :return: The urgent_pointer of this PacketProtocolTcp.  # noqa: E501
        :rtype: int
        """
        return self._urgent_pointer

    @urgent_pointer.setter
    def urgent_pointer(self, urgent_pointer):
        """Sets the urgent_pointer of this PacketProtocolTcp.

        tcp urgent pointer  # noqa: E501

        :param urgent_pointer: The urgent_pointer of this PacketProtocolTcp.  # noqa: E501
        :type: int
        """
        self._urgent_pointer = urgent_pointer

    @property
    def window(self):
        """Gets the window of this PacketProtocolTcp.  # noqa: E501

        tcp window  # noqa: E501

        :return: The window of this PacketProtocolTcp.  # noqa: E501
        :rtype: int
        """
        return self._window

    @window.setter
    def window(self, window):
        """Sets the window of this PacketProtocolTcp.

        tcp window  # noqa: E501

        :param window: The window of this PacketProtocolTcp.  # noqa: E501
        :type: int
        """
        self._window = window

    def to_dict(self):
        """Returns the model properties as a dict"""
        result = {}

        for attr, _ in six.iteritems(self.swagger_types):
            value = getattr(self, attr)
            if isinstance(value, list):
                result[attr] = list(map(
                    lambda x: x.to_dict() if hasattr(x, "to_dict") else x,
                    value
                ))
            elif hasattr(value, "to_dict"):
                result[attr] = value.to_dict()
            elif isinstance(value, dict):
                result[attr] = dict(map(
                    lambda item: (item[0], item[1].to_dict())
                    if hasattr(item[1], "to_dict") else item,
                    value.items()
                ))
            else:
                result[attr] = value
        if issubclass(PacketProtocolTcp, dict):
            for key, value in self.items():
                result[key] = value

        return result

    def to_str(self):
        """Returns the string representation of the model"""
        return pprint.pformat(self.to_dict())

    def __repr__(self):
        """For `print` and `pprint`"""
        return self.to_str()

    def __eq__(self, other):
        """Returns true if both objects are equal"""
        if not isinstance(other, PacketProtocolTcp):
            return False

        return self.__dict__ == other.__dict__

    def __ne__(self, other):
        """Returns true if both objects are not equal"""
        return not self == other