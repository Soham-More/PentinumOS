#include "usb.hpp"

#include <io/io.h>

#include <std/std.hpp>
#include <std/ds.hpp>

#include <cpu/exceptions.hpp>

namespace drivers::usb
{
    struct config_desc_ctrlin
    {
        u8  length;
        u8  descType;
        u16 totalLength;
        u8  interfaceCount;
        u8  value;
        u8  index;
        u8  attribs;
        u8  maxPower;
    }_packed;
    struct interface_desc_ctrlin
    {
        u8  length;
        u8  descType;
        u8  interfaceID;
        u8  alternateSetting;
        u8  endpointCount;
        u8  classCode;
        u8  subClass;
        u8  protocolCode;
        u8  index;
    }_packed;
    struct endpoint_desc_ctrlin
    {
        u8  length;
        u8  descType;
        u8  endpointAddress;
        u8  attributes;
        u16 maxPacketSize;
        u8  interval;
    }_packed;

    std::string get_string(const usb_device &device, usb_controller* controller, u8 stringIndex)
    {
        if(!stringIndex) return std::string();

        if(controller == nullptr)
        {
            log_error("[USB][Protocol Layer] device has null controller\n");
            x86_raise(0);
        }

        u8 stringSize = 0;

        // get string size
        request_packet reqPacket;
        reqPacket.requestType = USB_DEVICE_TO_HOST;
        reqPacket.request = USB_GET_DESC;
        reqPacket.value = (USB_DESC_STRING << 8) | stringIndex;
        reqPacket.index = LanguageID;
        reqPacket.size = 1;

        controller->controlIn(controller->instance_data, device, reqPacket, &stringSize, 1);

        // allocate buffer
        u16* wideBuffer = (u16*)std::malloc(stringSize);

        // get the string
        reqPacket.requestType = USB_DEVICE_TO_HOST;
        reqPacket.request = USB_GET_DESC;
        reqPacket.value = (USB_DESC_STRING << 8) | stringIndex;
        reqPacket.index = LanguageID;
        reqPacket.size = stringSize;

        controller->controlIn(controller->instance_data, device, reqPacket, wideBuffer, stringSize);

        //copyWideStringToString(wideBuffer + 1, buffer, stringSize - 2);

        std::string value(wideBuffer + 1, stringSize - 2);

        std::free(wideBuffer);

        return value;
    }

    config_desc get_config(vfs::node_t* dnode, u8 configID)
    {
        const usb_device& device = *reinterpret_cast<const usb_device*>(dnode->data);
        usb_controller* controller = reinterpret_cast<usb_controller*>(dnode->parent->data);

        if(controller == nullptr)
        {
            log_error("[USB][Protocol Layer] device has null controller\n");
            x86_raise(0);
        }

        config_desc configDesc;
        config_desc_ctrlin configState;

        // get configuration details
        request_packet reqPacket;
        reqPacket.requestType = USB_DEVICE_TO_HOST;
        reqPacket.request = USB_GET_DESC;
        reqPacket.value = (USB_DESC_CONFIG << 8) | 0;
        reqPacket.index = 0;
        reqPacket.size = sizeof(config_desc_ctrlin);

        bool status = controller->controlIn(controller->instance_data, device, reqPacket, &configState, sizeof(config_desc_ctrlin));

        // set configDesc values
        configDesc.interfaceCount = configState.interfaceCount;
        configDesc.attributes = configState.attribs;
        configDesc.maxPower = configState.maxPower;
        configDesc.value = configState.value;

        // allocate enough for getting full config info
        u8* buffer = (u8*)std::malloc(configState.totalLength);

        // send another request for getting all the interfaces associated with
        // this configuration
        reqPacket.requestType = USB_DEVICE_TO_HOST;
        reqPacket.request = USB_GET_DESC;
        reqPacket.value = (USB_DESC_CONFIG << 8) | 0;
        reqPacket.index = 0;
        reqPacket.size = configState.totalLength;
        status &= controller->controlIn(controller->instance_data, device, reqPacket, buffer, configState.totalLength);

        // allocate interfaces
        configDesc.interfaces = new interface_desc[configState.interfaceCount];

        // parse all interfaces and endpoints
        u8* interfacePtr = buffer + sizeof(config_desc_ctrlin);

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

            currentInterface->endpoints = new endpoint_desc[interface->endpointCount];

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
            currentInterface->interfaceDesc = get_string(device, controller, interface->index);
        }

        configDesc.configDesc = get_string(device, controller, configState.index);

        std::free(buffer);

        return configDesc;
    }
    void free_config(config_desc& configDesc)
    {
        for(size_t i = 0; i < configDesc.interfaceCount; i++)
        {
            // free all endpoints
            delete[] configDesc.interfaces[i].endpoints;
            configDesc.interfaces[i].endpoints = 0;
        }

        // free interface
        delete[] configDesc.interfaces;
        configDesc.interfaces = 0;
    }

    bool set_config(vfs::node_t* dnode, const config_desc& config)
    {
        const usb_device& device = *reinterpret_cast<const usb_device*>(dnode->data);
        usb_controller* controller = reinterpret_cast<usb_controller*>(dnode->parent->data);

        if(controller == nullptr)
        {
            log_error("[USB][Protocol Layer] device has null controller\n");
            x86_raise(0);
        }

        // set configuration packet
        request_packet reqPacket;
        reqPacket.requestType = USB_HOST_TO_DEVICE | USB_REQ_STRD | USB_REP_DEVICE;
        reqPacket.request = USB_SET_CONFIG;
        reqPacket.value = config.value;
        reqPacket.index = 0;
        reqPacket.size = 0;

        bool status = controller->controlOut(controller->instance_data, device, reqPacket, 0);

        return status;
    }
    u8 get_device_cfg_id(vfs::node_t* dnode)
    {
        const usb_device& device = *reinterpret_cast<const usb_device*>(dnode->data);
        usb_controller* controller = reinterpret_cast<usb_controller*>(dnode->parent->data);
        if(controller == nullptr)
        {
            log_error("[USB][Protocol Layer] device has null controller\n");
            x86_raise(0);
        }

        // set configuration packet
        request_packet reqPacket;
        reqPacket.requestType = USB_DEVICE_TO_HOST | USB_REQ_STRD | USB_REP_DEVICE;
        reqPacket.request = USB_GET_CONFIG;
        reqPacket.value = 0;
        reqPacket.index = 0;
        reqPacket.size = 1;

        u8 configID = 0;

        bool status = controller->controlIn(controller->instance_data, device, reqPacket, &configID, 1);

        return configID;
    }

    bool set_interface(vfs::node_t* dnode, const interface_desc& interface)
    {
        const usb_device& device = *reinterpret_cast<const usb_device*>(dnode->data);
        usb_controller* controller = reinterpret_cast<usb_controller*>(dnode->parent->data);
        if(controller == nullptr)
        {
            log_error("[USB][Protocol Layer] device has null controller\n");
            x86_raise(0);
        }

        // set interface packer
        request_packet reqPacket;
        reqPacket.requestType = USB_HOST_TO_DEVICE | USB_REQ_STRD | USB_REP_DEVICE;
        reqPacket.request = USB_SET_CONFIG;
        reqPacket.value = interface.alternateSetting;
        reqPacket.index = interface.interfaceID;
        reqPacket.size = 0;

        bool status = controller->controlOut(controller->instance_data, device, reqPacket, 0);

        return status;
    }

    bool clear_feature(vfs::node_t* dnode, u8 feature, u8 featureSelection, u8 featureID)
    {
        const usb_device& device = *reinterpret_cast<const usb_device*>(dnode->data);
        usb_controller* controller = reinterpret_cast<usb_controller*>(dnode->parent->data);
        if(controller == nullptr)
        {
            log_error("[USB][Protocol Layer] device has null controller\n");
            x86_raise(0);
        }

        // clear feature packer
        request_packet reqPacket;
        reqPacket.requestType = USB_HOST_TO_DEVICE | USB_REQ_STRD | (feature & 0b11);
        reqPacket.request = USB_CLEAR_FEATURE;
        reqPacket.value = featureSelection & 0b11;
        reqPacket.index = featureID;
        reqPacket.size = 0;

        bool status = controller->controlOut(controller->instance_data, device, reqPacket, 0);

        return status;
    }

    // send control packet to device
    bool control_packet_out(vfs::node_t* dnode, request_packet rPacket)
    {
        const usb_device& device = *reinterpret_cast<const usb_device*>(dnode->data);
        usb_controller* controller = reinterpret_cast<usb_controller*>(dnode->parent->data);
        if(controller == nullptr)
        {
            log_error("[USB][Protocol Layer] device has null controller\n");
            x86_raise(0);
        }

        return controller->controlOut(controller->instance_data, device, rPacket, rPacket.size);
    }
    // get control packet to device
    bool control_packet_in(vfs::node_t* dnode, request_packet rPacket, void* buffer)
    {
        const usb_device& device = *reinterpret_cast<const usb_device*>(dnode->data);
        usb_controller* controller = reinterpret_cast<usb_controller*>(dnode->parent->data);
        if(controller == nullptr)
        {
            log_error("[USB][Protocol Layer] device has null controller\n");
            x86_raise(0);
        }

        return controller->controlIn(controller->instance_data, device, rPacket, buffer, rPacket.size);
    }

    // bulk out
    bool bulk_out(vfs::node_t* dnode, endpoint_desc* endpoint, void* buffer, size_t size)
    {
        const usb_device& device = *reinterpret_cast<const usb_device*>(dnode->data);
        usb_controller* controller = reinterpret_cast<usb_controller*>(dnode->parent->data);
        if(controller == nullptr)
        {
            log_error("[USB][Protocol Layer] device has null controller\n");
            x86_raise(0);
        }

        return controller->bulkOut(controller->instance_data, device, endpoint->endpointAddress & 0xF, endpoint->maxPacketSize, buffer, size);
    }
    bool bulk_in(vfs::node_t* dnode, endpoint_desc* endpoint, void* buffer, size_t size)
    {
        const usb_device& device = *reinterpret_cast<const usb_device*>(dnode->data);
        usb_controller* controller = reinterpret_cast<usb_controller*>(dnode->parent->data);
        if(controller == nullptr)
        {
            log_error("[USB][Protocol Layer] device has null controller\n");
            x86_raise(0);
        }

        return controller->bulkIn(controller->instance_data, device, endpoint->endpointAddress & 0xF, endpoint->maxPacketSize, buffer, size);
    }
}
