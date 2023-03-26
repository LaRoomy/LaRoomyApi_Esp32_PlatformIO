#include <LaRoomyApi_Esp32.h>

// Check out the documentation at: https://api.laroomy.com/

/**
 * NOTE:    In this example the 'PinStorageController' class is used to handle the memory storage of
 *          the pin. But this class is a singleton instance and does not cover the case when there are more than
 *          one UnlockControl property. In this scenario the 'PinStorageController' is not suitable
 *          and a custom implementation of pin storage is necessary.
*/

// define the pin names
#define CON_LED 2 // connection indicator led
#define LED_1 4   // led 1
#define LED_2 5   // led 2
#define HBUTTON_1 18 // hardware button 1

// define the control IDs - IMPORTANT: do not use zero as ID value and do not use an ID value more than once !!
#define SP_UNLOCK_CONTROL 1
#define SP_NAVIGATOR 2

#define MAXIMUM_UNLOCK_ATTEMPTS 3

// counter for failed unlock attempts
unsigned int unlockAttempts = 0;

// define the callback for the remote app-user events - https://api.laroomy.com/p/laroomy-app-callback.html
class RemoteEvents : public ILaroomyAppCallback
{
public:
    // receive connection state change info
    void onConnectionStateChanged(bool newState) override
    {
        if (newState)
        {
            Serial.println("Connected!");
            digitalWrite(CON_LED, HIGH);
        }
        else
        {
            Serial.println("Disconnected!");
            digitalWrite(CON_LED, LOW);

            // reset params
            unlockAttempts = 0;
        }
    }

    // catch the state change of the unlock control property
    void onUnlockControlStateChanged(cID unlockControlID, const UnlockControlState &state) override
    {
        // in unlock mode, lock or unlock whatever is to be unlocked
        if (state.mode == UnlockControlModes::UNLOCK_MODE)
        {
            if (state.unlocked)
            {
                digitalWrite(LED_1, LOW);
                digitalWrite(LED_2, HIGH);
                Serial.println("Unlocked!");
            }
            else
            {
                digitalWrite(LED_1, HIGH);
                digitalWrite(LED_2, LOW);
                Serial.println("Locked!");
            }

            // reset param
            unlockAttempts = 0;
        }
        else if (state.mode == UnlockControlModes::PIN_CHANGE_MODE)
        {
            /*
                Pin changes are handled internally, this callback only reports if the pin was changed
                    NOTE:   The pin must be saved permanently and must be reloaded on power-loss to remain consistent.
                            Here this is realized with the PinStorageController object which implements the whole
                            functionality.

                        https://api.laroomy.com/p/helper-classes.html#laroomyApiRefMIDPSCtrler
            */

            // save the pin
            if (PinStorageController.savePin(state.pin))
            {
                Serial.print("Pin saved: ");
                Serial.println(state.pin);
            }
            else
            {
                Serial.println("ERROR: Failed to save pin!");
            }

            // reset param
            unlockAttempts = 0;
        }
    }

    // catch an invalid operation on the unlock control property
    void onUnlockControlInvalidOperation(cID unlockControlID, UnlockControlInvalidOperation invalidOperation) override
    {
        if (invalidOperation == UnlockControlInvalidOperation::PIN_CHANGE_REJECTED_WRONG_PIN)
        {
            Serial.println("Unlock-Control - Pin change failed: wrong pin!");
        }
        else if (invalidOperation == UnlockControlInvalidOperation::UNLOCK_FAILED_WRONG_PIN)
        {
            Serial.println("Unlock-Control - Unlock failed: wrong pin!");
        }

        if ((invalidOperation == UnlockControlInvalidOperation::UNLOCK_FAILED_WRONG_PIN) ||
            (invalidOperation == UnlockControlInvalidOperation::PIN_CHANGE_REJECTED_WRONG_PIN))
        {
            // the app user has entered a wrong pin, so unlock failed, here is the possibility to
            // implement a maximum tryout for wrong pins

            unlockAttempts++;

            // max-attemts X times, then the property is blocked
            if (unlockAttempts >= MAXIMUM_UNLOCK_ATTEMPTS)
            {
                // the user has entered a wrong pin 3 times, navigate back to main and disable property
                // navigate back to device main page
                LaRoomyApi.sendNavBackToDeviceMainCommand();

                // disable property
                LaRoomyApi.disableProperty(unlockControlID);
            }
        }
    }

    // catch the navigator state changes
    void onNavigatorStateChanged(cID navigatorID, const NavigatorState &state) override
    {
        Serial.print("ButtonType:  ");

        switch (state.buttonType)
        {
        case NavigatorButtonTypes::UP_BUTTON:
            Serial.print("UP");
            break;
        case NavigatorButtonTypes::RIGHT_BUTTON:
            Serial.print("RIGHT");
            break;
        case NavigatorButtonTypes::DOWN_BUTTON:
            Serial.print("DOWN");
            break;
        case NavigatorButtonTypes::LEFT_BUTTON:
            Serial.print("LEFT");
            break;
        case NavigatorButtonTypes::MID_BUTTON:
            Serial.print("MID");
            break;
        default:
            break;
        }

        Serial.print("  Action was:  ");

        switch (state.actionType)
        {
        case NavigatorActionTypes::BTOUCHED_DOWN:
            Serial.print("Touch");
            break;
        case NavigatorActionTypes::BRELEASED:
            Serial.print("Release");
            break;
        default:
            Serial.print("Unknown!");
            break;
        }
        Serial.print("\r\n");
    }
};

void setup()
{
    // put your setup code here, to run once:

    // monitor output for evaluation
    Serial.begin(115200);

    pinMode(CON_LED, OUTPUT);
    pinMode(LED_1, OUTPUT);
    pinMode(LED_2, OUTPUT);
    pinMode(HBUTTON_1, INPUT);


    digitalWrite(CON_LED, LOW);
    digitalWrite(LED_1, HIGH);
    digitalWrite(LED_2, LOW);

    // begin - https://api.laroomy.com/p/laroomy-api-class.html
    LaRoomyApi.begin();

    // set the bluetooth name
    LaRoomyApi.setBluetoothName("Esp32_Ble");

    // set the image for the device
    LaRoomyApi.setDeviceImage(LaRoomyImages::ACCESS_CONTROL_025);

    // set the callback handler for remote events
    LaRoomyApi.setCallbackInterface(
        dynamic_cast<ILaroomyAppCallback *>(
            new RemoteEvents()));

    // docu of the property classes: https://api.laroomy.com/p/property-classes.html

    // create the unlock control property
    UnlockControl uc;
    uc.imageID = LaRoomyImages::LOCK_CLOSED_021;
    uc.unlockControlDescription = "Unlock";
    uc.unlockControlID = SP_UNLOCK_CONTROL;

    /**
     * Load the previously saved pin from flash or set a default pin if it wasn't saved before.
     */
    uc.unlockControlState.pin = PinStorageController.loadPin();

    // check if the saved pin is successful loaded (LoadPin() returns default value '12345' if no pin is saved)
    Serial.print("Startup: Pin loaded: ");
    Serial.println(uc.unlockControlState.pin);

    // add the property
    LaRoomyApi.addDeviceProperty(uc);

    // create the navigator control
    NavigatorControl nc;
    nc.imageID = LaRoomyImages::NAVIGATOR_056;
    nc.navigatorDescription = "Navigate";
    nc.navigatorID = SP_NAVIGATOR;

    // add the property
    LaRoomyApi.addDeviceProperty(nc);

    // on the end, call run to apply the setup and start bluetooth advertising
    LaRoomyApi.run();
}

void loop()
{
    // put your main code here, to run repeatedly:

    // hardware button is pressed
    if (digitalRead(HBUTTON_1) == LOW)
    {
        delay(10);

        // if the unlock control property was blocked, enable it
        LaRoomyApi.enableProperty(SP_UNLOCK_CONTROL);
        
        // reset param 
        unlockAttempts = 0;

        while (digitalRead(HBUTTON_1) == LOW)
            ;
        delay(50);
    }
}