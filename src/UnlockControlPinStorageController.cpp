#include "UnlockControlPinStorageController.h"

bool UnlockControlPinStorageController::unlockControlPinStorageControllerInstanceCreated = false;
UnlockControlPinStorageController* UnlockControlPinStorageController::ucpInstance = nullptr;

UnlockControlPinStorageController::UnlockControlPinStorageController()
{}

UnlockControlPinStorageController::~UnlockControlPinStorageController()
{
    unlockControlPinStorageControllerInstanceCreated = false;
}

UnlockControlPinStorageController* UnlockControlPinStorageController::GetInstance()
{
    if(unlockControlPinStorageControllerInstanceCreated)
    {
        return ucpInstance;
    }
    else
    {
        ucpInstance = new UnlockControlPinStorageController();
        if(ucpInstance != nullptr)
        {
            unlockControlPinStorageControllerInstanceCreated = true;
        }
        return ucpInstance;
    }
}

String UnlockControlPinStorageController::loadPin() 
{
    // default value
    String pin("12345");

    esp_err_t res;

    // open
    res = nvs_open(UNLOCKCONTROL_STORAGE_NAMESPACE, NVS_READWRITE, &this->nvsDataHandle);
    if(res == ESP_OK){

        size_t required_size = 0;

        // read size of string
        res = nvs_get_str(this->nvsDataHandle, UNLOCKCONTROL_DATA_STORAGE_KEY, nullptr, &required_size);
        if(res == ESP_OK){

            // read data (if applicable)
            if(required_size > 0){

                // allocate mem
                char* data = new char[required_size + (size_t)1];
                if(data != nullptr){

                    res = nvs_get_str(this->nvsDataHandle, UNLOCKCONTROL_DATA_STORAGE_KEY, data, &required_size);
                    if(res == ESP_OK){

                        pin = data;
                    }
                    delete [] data;
                }
            }
        }
        nvs_close(this->nvsDataHandle);
    }
    return pin;
}

bool UnlockControlPinStorageController::savePin(const String& pin)
{
    // validate pin length
    if(pin.length() == 0){
        return false;
    }

    esp_err_t res;
    bool vRes = false;

    // open
    res = nvs_open(UNLOCKCONTROL_STORAGE_NAMESPACE, NVS_READWRITE, &this->nvsDataHandle);
    if(res == ESP_OK){

        // write
        res = nvs_set_str(this->nvsDataHandle, UNLOCKCONTROL_DATA_STORAGE_KEY, pin.c_str());
        if(res == ESP_OK){

            // commit
            res = nvs_commit(this->nvsDataHandle);
            if(res == ESP_OK){
                vRes = true;
            }
        }
        nvs_close(this->nvsDataHandle);
    }
    return vRes;
}