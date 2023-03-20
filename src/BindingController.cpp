#include "BindingController.h"

BindingController::BindingController(){
    this->init();
}

BindingController::~BindingController(){
    bindingControllerInstanceCreated = false;
}

BindingController* BindingController::GetInstance(){
    if(bindingControllerInstanceCreated){
        return bcInstance;
    }
    else {
        bcInstance = new BindingController();
        if(bcInstance != nullptr){
            bindingControllerInstanceCreated = true;
        }
        return bcInstance;
    }
}

void BindingController::init(){
    this->bKey.clear();
    this->readData();
}

bool BindingController::bindingControllerInstanceCreated = false;
BindingController* BindingController::bcInstance = nullptr;

BindingResponseType BindingController::handleBindingTransmission(BindingTransmissionTypes bType, const String& key){

    switch (bType)
    {
        case BindingTransmissionTypes::B_AUTH_REQUEST:
            if(this->bKey == key){
                // the key is valid
                return BindingResponseType::BINDING_AUTHENTICATION_SUCCESS;               
            }
            else {
                // key is invalid, auth fail
                return BindingResponseType::BINDING_AUTHENTICATION_FAIL_WRONG_KEY;
            }
        case BindingTransmissionTypes::B_ENABLE:
            // save the key + set required to true
            if(key.length() > 0){

                this->bKey = key;
                this->bRequired = true;

                // write nvs
                if(this->writeData()){
                    return BindingResponseType::BINDING_ENABLE_SUCCESS;
                }
                else {
                    return BindingResponseType::BINDING_ERROR_UNKNOWN;
                }
            }
            else {
                return BindingResponseType::BINDING_ERROR_UNKNOWN;
            }
        case BindingTransmissionTypes::B_RELEASE:

            // delete key + set required to false
            this->bKey.clear();
            this->bRequired = false;

            // erase nvs
            this->eraseData();

            return BindingResponseType::BINDING_RELEASE_SUCCESS;
        default:
            return BindingResponseType::BINDING_ERROR_UNKNOWN_REQUEST;
    }
}

bool BindingController::writeData(){
    // check if binding-key has content
    if(this->bKey.length() == 0){
        return false;
    }

    esp_err_t res;
    bool vRes = false;

    // open
    res = nvs_open(BINDING_STORAGE_NAMESPACE, NVS_READWRITE, &this->nvsDataHandle);
    if(res == ESP_OK){

        // write
        res = nvs_set_str(this->nvsDataHandle, BINDING_DATA_STORAGE_KEY, this->bKey.c_str());
        if(res == ESP_OK){

            // commit
            res = nvs_commit(this->nvsDataHandle);
            if(res == ESP_OK){
                vRes = true;
            }
        }
        // close & return
        nvs_close(this->nvsDataHandle);
    }
    return vRes;
}

bool BindingController::readData(){
    // erase key
    this->bKey.clear();

    esp_err_t res;
    bool vRes = false;

    // open
    res = nvs_open(BINDING_STORAGE_NAMESPACE, NVS_READWRITE, &this->nvsDataHandle);
    if(res == ESP_OK){

        size_t required_size = 0;

        // read size of string
        res = nvs_get_str(this->nvsDataHandle, BINDING_DATA_STORAGE_KEY, nullptr, &required_size);
        if(res == ESP_OK){

            // only proceed if size is above zero
            if(required_size > 0){

                // allocate mem
                char* data = new char[required_size + (size_t)1];
                if(data != nullptr){
                    
                    // read data
                    res = nvs_get_str(this->nvsDataHandle, BINDING_DATA_STORAGE_KEY, data, &required_size);
                    if(res == ESP_OK){

                        vRes = true;
                        this->bKey = data;
                        this->bRequired = true;
                    }
                    delete []data;
                }
            }
        }
        nvs_close(this->nvsDataHandle);
    }
    return vRes;
}

bool BindingController::eraseData(){

    esp_err_t res;
    bool vRes = false;

    // open
    res = nvs_open(BINDING_STORAGE_NAMESPACE, NVS_READWRITE, &this->nvsDataHandle);
    if(res == ESP_OK){
    
        // erase
        res = nvs_erase_key(this->nvsDataHandle, BINDING_DATA_STORAGE_KEY);
        if(res == ESP_OK) { // res == ESP_ERR_NVS_NOT_FOUND if key does not exist

            // commit
            res = nvs_commit(this->nvsDataHandle);
            if(res == ESP_OK){
                vRes = true;
            }
        }
        // close & return
        nvs_close(this->nvsDataHandle);
    }
    return vRes;
}
