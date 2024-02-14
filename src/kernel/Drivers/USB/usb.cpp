#include "usb.hpp"
#include <std/string.h>
#include <std/Heap/heap.hpp>

namespace USB
{
    struct config_desc_ctrlin
    {
        uint8_t  length;
        uint8_t  descType;
        uint16_t totalLength;
        uint8_t  interfaceCount;
        uint8_t  value;
        uint8_t  index;
        uint8_t  attribs;
        uint8_t  maxPower;
    }_packed;
    struct interface_desc_ctrlin
    {
        uint8_t  length;
        uint8_t  descType;
        uint8_t  interfaceID;
        uint8_t  alternateSetting;
        uint8_t  endpointCount;
        uint8_t  classCode;
        uint8_t  subClass;
        uint8_t  protocolCode;
        uint8_t  index;
    }_packed;
    struct endpoint_desc_ctrlin
    {
        uint8_t  length;
        uint8_t  descType;
        uint8_t  endpointAddress;
        uint8_t  attributes;
        uint16_t maxPacketSize;
        uint8_t  interval;
    }_packed;

    USBController* currentController;

    void setController(USBController& controller)
    {
        currentController = &controller;
    }
    USBController* getController()
    {
        return currentController;
    }

    char* getString(const usb_device& device, uint8_t stringIndex)
    {
        if(!stringIndex) return nullptr;

        uint8_t stringSize = 0;

        // get string size
        USB::request_packet reqPacket;
        reqPacket.requestType = USB_DEVICE_TO_HOST;
        reqPacket.request = USB_GET_DESC;
        reqPacket.value = (USB_DESC_STRING << 8) | stringIndex;
        reqPacket.index = LanguageID;
        reqPacket.size = 1;

        currentController->controlIn(device, reqPacket, &stringSize, 1);

        // allocate buffer
        uint16_t* wideBuffer = (uint16_t*)std::malloc(stringSize);
        char* buffer = (char*)std::malloc(stringSize / 2);

        // get the string
        reqPacket.requestType = USB_DEVICE_TO_HOST;
        reqPacket.request = USB_GET_DESC;
        reqPacket.value = (USB_DESC_STRING << 8) | stringIndex;
        reqPacket.index = LanguageID;
        reqPacket.size = stringSize;

        currentController->controlIn(device, reqPacket, wideBuffer, stringSize);

        copyWideStringToString(wideBuffer + 1, buffer, stringSize - 2);

        std::free(wideBuffer);

        return buffer;
    }
    void freeString(char** string)
    {
        std::free(*string);
        *string = 0;
    }

    config_desc getConfig(const usb_device& device, uint8_t configID)
    {
        config_desc configDesc;

        config_desc_ctrlin configState;

        // get configuration details
        request_packet reqPacket;
        reqPacket.requestType = USB_DEVICE_TO_HOST;
        reqPacket.request = USB_GET_DESC;
        reqPacket.value = (USB_DESC_CONFIG << 8) | 0;
        reqPacket.index = 0;
        reqPacket.size = sizeof(config_desc_ctrlin);

        bool status = currentController->controlIn(device, reqPacket, &configState, sizeof(config_desc_ctrlin));

        // set configDesc values
        configDesc.interfaceCount = configState.interfaceCount;
        configDesc.attributes = configState.attribs;
        configDesc.maxPower = configState.maxPower;
        configDesc.value = configState.value;

        // allocate enough for getting full config info
        uint8_t* buffer = (uint8_t*)std::malloc(configState.totalLength);

        // send another request for getting all the interfaces associated with
        // this configuration
        reqPacket.requestType = USB_DEVICE_TO_HOST;
        reqPacket.request = USB_GET_DESC;
        reqPacket.value = (USB_DESC_CONFIG << 8) | 0;
        reqPacket.index = 0;
        reqPacket.size = configState.totalLength;
        status &= currentController->controlIn(device, reqPacket, buffer, configState.totalLength);

        // allocate interfaces
        configDesc.interfaces = (interface_desc*)std::malloc(configState.interfaceCount * sizeof(interface_desc));

        // parse all interfaces and endpoints
        uint8_t* interfacePtr = buffer + sizeof(config_desc_ctrlin);

        for(size_t i = 0; i < configState.interfaceCount; i++)
        {
            interface_desc_ctrlin* interface = (interface_desc_ctrlin*)interfacePtr;
            interface_desc* currentInterface = &configDesc.interfaces[i];

            currentInterface->alternateSetting = interface->alternateSetting;
            currentInterface->endpointCount = interface->endpointCount;
            currentInterface->protocolCode = interface->protocolCode;
            currentInterface->interfaceID = interface->interfaceID;
            currentInterface->classCode = interface->classCode;
            currentInterface->subClass = interface->subClass;

            currentInterface->endpoints = (endpoint_desc*)std::malloc(interface->endpointCount * sizeof(endpoint_desc));

            interfacePtr += sizeof(interface_desc_ctrlin);

            for(size_t j = 0; j < interface->endpointCount; j++)
            {
                endpoint_desc* currentEndpoint = &currentInterface->endpoints[j];
                endpoint_desc_ctrlin* endpoint = (endpoint_desc_ctrlin*)interfacePtr;

                currentEndpoint->endpointAddress = endpoint->endpointAddress;
                currentEndpoint->maxPacketSize = endpoint->maxPacketSize;
                currentEndpoint->attributes = endpoint->attributes;
                currentEndpoint->interval = endpoint->interval;

                interfacePtr += sizeof(endpoint_desc_ctrlin);
            }

            // get string desciptor, if index is non zero
            currentInterface->interfaceDesc = getString(device, interface->index);
        }

        configDesc.configDesc = getString(device, configState.index);

        std::free(buffer);

        return configDesc;
    }
    void freeConfig(config_desc& configDesc)
    {
        for(size_t i = 0; i < configDesc.interfaceCount; i++)
        {
            // free all endpoints
            std::free(configDesc.interfaces[i].endpoints);
            configDesc.interfaces[i].endpoints = 0;

            if(configDesc.interfaces[i].interfaceDesc != nullptr)
            {
                freeString(&configDesc.interfaces[i].interfaceDesc);
            }
        }

        // free interface
        std::free(configDesc.interfaces);
        configDesc.interfaces = 0;
    }
}
