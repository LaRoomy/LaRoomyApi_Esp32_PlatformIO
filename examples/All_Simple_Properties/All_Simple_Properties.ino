#include <LaRoomyApi_Esp32.h>

// Check out the documentation: https://api.laroomy.com/

// define the pin names
#define CON_LED 2    // connection indicator led
#define HBUTTON_1 18 // hardware button 1
#define HBUTTON_2 19 // hardware button 2
#define LED_1 4      // led 1
#define LED_2 5      // led 2
#define POTI_1 15

// define the control IDs - NOTE: do not use zero as ID value and do not use an ID value more than once !!
#define SP_BUTTON_ID 1
#define SP_SWITCH_ID 2
#define SP_LEVEL_SELECTOR_ID 3
#define SP_LEVEL_INDICATOR_ID 4
#define SP_TEXT_DISPLAY_ID 5
#define SP_OPTION_SELECTOR_ID 6

// timer params
long mTimer = 0;
long mTimeOut = 100;

// adc holder
int adc_val = 0;
int old_adc = 0;

int adc_read_average(uint8_t pin);

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
        }
    }

    // receive button pressed event
    void onButtonPressed(cID buttonID) override
    {
        // here is a demonstation on how to use a simple text property to display a message:

        // first get the property
        TextDisplay td(SP_TEXT_DISPLAY_ID);

        // change the property params
        if (td.imageID == LaRoomyImages::WARNING_NORMAL_144)
        {
            td.imageID = LaRoomyImages::NOTIFICATION_067;
            td.textToDisplay = "Status: Ready";
        }
        else
        {
            td.imageID = LaRoomyImages::WARNING_NORMAL_144;
            td.textToDisplay = "Warning: some message...";
        }

        // call update to apply the property changes
        td.update();
    }

    // receive switch state changes
    void onSwitchStateChanged(cID switchID, bool newState) override
    {
        digitalWrite(LED_1, newState ? HIGH : LOW);
    }

    // receive level-selector value changes
    void onLevelSelectorValueChanged(cID levelSelectorID, unsigned int newValue) override
    {
        if (levelSelectorID == SP_LEVEL_SELECTOR_ID)
        {
            analogWrite(LED_2, (int)newValue);
        }
    }

    // receive index changes of the option selector
    void onOptionSelectorIndexChanged(cID optionSelectorID, unsigned int newIndex) override
    {
        String msg("Option-Selector index changed to: ");
        msg += newIndex;
        Serial.println(msg.c_str());
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
    pinMode(HBUTTON_2, INPUT);

    digitalWrite(CON_LED, LOW);
    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, LOW);

    pinMode(POTI_1, INPUT);

    analogWrite(LED_2, 0);

    // begin - https://api.laroomy.com/p/laroomy-api-class.html
    LaRoomyApi.begin();

    // set the bluetooth name
    LaRoomyApi.setBluetoothName("Esp32_Ble");

    // set the image for the device in the device list
    LaRoomyApi.setDeviceImage(LaRoomyImages::HOUSE_CONTROL_160);

    // set the callback handler for remote events
    LaRoomyApi.setCallbackInterface(
        dynamic_cast<ILaroomyAppCallback *>(
            new RemoteEvents()));

    // docu for property classes: https://api.laroomy.com/p/property-classes.html

    // define and insert a button
    Button btn;
    btn.buttonID = SP_BUTTON_ID;
    btn.imageID = LaRoomyImages::LIGHT_BULB_004;
    btn.buttonDescriptor = "Push the";
    btn.buttonText = "Button";
    LaRoomyApi.addDeviceProperty(btn);

    // define and insert a switch
    Switch sw;
    sw.switchID = SP_SWITCH_ID;
    sw.imageID = LaRoomyImages::FAN_134;
    sw.switchDescription = "Switch Control";
    sw.switchState = OFF;
    LaRoomyApi.addDeviceProperty(sw);

    // define and insert a level selector
    LevelSelector ls;
    ls.levelSelectorID = SP_LEVEL_SELECTOR_ID;
    ls.imageID = LaRoomyImages::LEVEL_ADJUST_043;
    ls.level = 0;
    ls.levelSelectorDescription = "Select the Level";
    LaRoomyApi.addDeviceProperty(ls);

    // define and insert a level indicator
    LevelIndicator li;
    li.levelIndicatorID = SP_LEVEL_INDICATOR_ID;
    li.imageID = LaRoomyImages::BATTERY_100P_032;
    li.level = 0;
    li.valueColor = Colors::PaleRed;
    li.levelIndicatorDescription = "Battery Level";
    LaRoomyApi.addDeviceProperty(li);

    // define and insert a text display
    TextDisplay td;
    td.textDisplayID = SP_TEXT_DISPLAY_ID;
    td.imageID = LaRoomyImages::NOTIFICATION_067;
    td.textToDisplay = "Status: Ready";
    LaRoomyApi.addDeviceProperty(td);

    // define and insert a option selector
    OptionSelector os;
    os.optionSelectorID = SP_OPTION_SELECTOR_ID;
    os.imageID = LaRoomyImages::AIR_CIRCULATION_142;
    os.selectedIndex = 0;
    os.optionSelectorDescription = "Grade select";
    os.addOption("Slow"); // add the options for the selector
    os.addOption("Mid");
    os.addOption("Fast");
    LaRoomyApi.addDeviceProperty(os);

    // finally, call run to apply the setup and start bluetooth advertising
    LaRoomyApi.run();

    // update timer
    mTimer = millis();
}

void loop()
{
    // put your main code here, to run repeatedly:

    /*
      An important point is: if there are properties whose states can change by remote side and
      device side, there is the possibility that the state(s) changes while the app is connected. In this case
      it is important to update the state, otherwise this breaks the hardware - app state alignment.
      for example: if a LED is switched of by hardware button the state of the appropriate switch in the app
      remains in the ON-state.
      To prevent this, update the internal state every time the regarding hardware element changes:
    */

    // first hardware button is pressed
    if (digitalRead(HBUTTON_1) == LOW)
    {
        delay(10);

        auto newState = digitalRead(LED_1) ? LOW : HIGH;

        digitalWrite(LED_1, newState);
        /*  This is a hardware event which modifies the device-state -> to make sure that the app-state conforms with
            the state of the device we have to update the state
        */
        LaRoomyApi.updateSimplePropertyState(SP_SWITCH_ID, (unsigned int)newState);

        while (digitalRead(HBUTTON_1) == LOW)
            ;
        delay(50);
    }

    // second hardware button is pressed
    if (digitalRead(HBUTTON_2) == LOW)
    {
        delay(10);

        // this is a demonstation of how to display a message to the app-user
        LaRoomyApi.sendUserMessage(UserMessageType::Info, UserMessageHoldingPeriod::FIVE_SECONDS, "The second hardware-button was pressed.");

        while (digitalRead(HBUTTON_2) == LOW)
            ;
        delay(50);
    }

    // measure analog input
    if (millis() > (unsigned long)(mTimeOut + mTimer))
    {
        /* NOTE:
            Do not update the parameter on every measure result, do only update when necessary and make sure there is a upper
            limit for update-transmissions. In this case the maximum update-transmission count is limited to 10 per second
            by a timer implementation.
            This assures that the transmission buffer will not be blocked by to many transmissions but ensures a fluent
            visible update of the app element on parameter changes!
        */

        // update timer reference
        mTimer = millis();

        // get the current value
        adc_val = adc_read_average(POTI_1);

        // update with a little hysteresis
        bool update = false;

        if (old_adc >= 30 && old_adc <= 4065)
        {
            if ((adc_val < (old_adc - 30)) || (adc_val > (old_adc + 30)))
            {
                update = true;
            }
            else
            {
                if (adc_val < 30)
                {
                    adc_val = 0;
                    update = true;
                }
                else if (adc_val > 4065)
                {
                    adc_val = 4095;
                    update = true;
                }
            }
        }
        else
        {
            if (adc_val > 30 && adc_val < 4095)
            {
                update = true;
            }
        }

        // update the property state if applicable
        if (update)
        {
            // save old adc value
            old_adc = adc_val;

            // update the internal state:
            
            /*
                here a simple state-update could be posted to change the current value of the property:

                pLr->UpdateSimplePropertyState(SP_LEVEL_INDICATOR_ID, adc_val / 16);

                but the value-color should also be changed, so there is a full property-update necessary
            */
            LevelIndicator li(SP_LEVEL_INDICATOR_ID);
            li.level = (uint8_t)(adc_val / 16);

            if (li.level < 25) // < 10%
            {
                li.valueColor = Colors::PaleRed;
            }
            else if(li.level < 63) // < 25%
            {
                li.valueColor = Colors::Yellow;
            }
            else
            {
                li.valueColor = Colors::LightGreen;
            }
            li.update();
            

            // monitor the value (for evaluation)
            Serial.print("Measured value: ");
            Serial.print(adc_val);
            Serial.print("\r\n");
        }
    }
}

int adc_read_average(uint8_t pin)
{
    int val = 0;
    for (unsigned int i = 0; i < 5; i++)
    {
        val += analogRead(pin);
    }
    return val / 5;
}