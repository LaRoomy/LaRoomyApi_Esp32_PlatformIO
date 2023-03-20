#ifndef BINDING_CONTROLLER_H
#define BINDING_CONTROLLER_H

#include <Arduino.h>
#include <nvs.h>

#include "constDef.h"

#define BINDING_STORAGE_NAMESPACE   "lr_bind_nspc_"
#define BINDING_DATA_STORAGE_KEY   "bnd_str_key_"

/**
 * @brief Implementation of the managed handling of the authentication, enable and release -process of the device-binding.
 */
class BindingController {
public:
    ~BindingController();

    /**
     * @brief Get the Singleton-Instance object of the BindingController class
     * 
     * @return BindingController* 
     */
    static BindingController* GetInstance();

    /**
     * @brief Binding control handler.
     * This method must be implemented as the response to the callback method 'onBindingTransmissionReceived' in the class derived from 'ILaroomyAppCallback'.
     * It takes the input parameter from this method, processes the storing of the binding data and returns the appropriate result (BindingResponseType).
     * 
     * @param bType The BindingTransmissionType
     * @param key The binding key
     * @return BindingResponseType 
     */
    BindingResponseType handleBindingTransmission(BindingTransmissionTypes bType, const String& key);

    /**
     * @brief Get the reqired status of the device binding.
     * 
     * @return Boolean - The required status
     */
    inline bool isBindingRequired(){
        return this->bRequired;
    }

private:
    BindingController();

    bool bRequired = false;
    String bKey;

    static bool bindingControllerInstanceCreated;
    static BindingController* bcInstance;

    nvs_handle nvsDataHandle;

    void init();

    bool writeData();
    bool readData();
    bool eraseData();
};

/**
 * @brief Access the binding controller helper API for the authentication, enable and release -process of the device-binding.
 *        This class takes also responsibility for storing the underlying data to flash.
 * 
 */
#define DeviceBindingController (*BindingController::GetInstance())

#endif