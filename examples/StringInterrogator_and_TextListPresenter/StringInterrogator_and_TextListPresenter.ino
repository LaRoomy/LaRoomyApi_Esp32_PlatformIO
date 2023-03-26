#include <LaRoomyApi_Esp32.h>

// Check out the documentation at: https://api.laroomy.com/

// define the pin names
#define CON_LED 2    // connection indicator led
#define HBUTTON_1 18 // hardware button 1
#define HBUTTON_2 19 // hardware button 2

// define the control IDs - IMPORTANT: do not use zero as ID value and do not use an ID value more than once !!
#define SP_LOGIN_PROPERTY 1
#define SP_LOGDATA_PROPERTY 2

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

    // receive string-interrogator confirm button clicks
    void onStringInterrogatorDataReceived(cID stringInterrogatorID, String &fieldOneContent, String &fieldTwoContent) override
    {
        // if a login attempt is executed, this callback method is called with the entered contents
        // -> here a log message is added to demonstrate both complex properties
        String msg = "Login attempt occurred. Username: ";

        if (fieldOneContent.length() > 0)
        {
            msg += fieldOneContent;
        }
        else
        {
            msg += "<empty>";
        }
        msg += " / Password: ";

        if (fieldTwoContent.length() > 0)
        {
            msg += fieldTwoContent;
        }
        else
        {
            msg += "<empty>";
        }

        // add message to text-list-presenter stack
        LaRoomyApi.addTextListPresenterElement(SP_LOGDATA_PROPERTY, TextListPresenterElementType::WARNING_MESSAGE, msg);

        // monitor
        Serial.println(msg);
    }

    void onPropertyLoadingComplete(PropertyLoadingType plt) override
    {
        LaRoomyApi.addTextListPresenterElement(SP_LOGDATA_PROPERTY, TextListPresenterElementType::DEFAULT_MESSAGE, "Hello user! This is a default-type message.");
    }

    // when the app was left without navigating back and the user goes back to it, this method is called
    void onDeviceConnectionRestored(cID currentOpenedPropertyPageID) override
    {
        // add as log message for demonstration
        LaRoomyApi.addTextListPresenterElement(SP_LOGDATA_PROPERTY, TextListPresenterElementType::INFO_MESSAGE, "Device connection restored!");
    }

    // detect when a complex property page is invoked
    void onComplexPropertyPageInvoked(cID propertyID) override
    {
        String msg = "You navigated to complex property page with ID: ";
        msg += (unsigned int)propertyID;
        LaRoomyApi.addTextListPresenterElement(SP_LOGDATA_PROPERTY, TextListPresenterElementType::DEFAULT_MESSAGE, msg);
    }

    // detect when the user navigated back to the device-main-page
    void onBackNavigation() override
    {
        LaRoomyApi.addTextListPresenterElement(SP_LOGDATA_PROPERTY, TextListPresenterElementType::DEFAULT_MESSAGE, "Back navigation occurred.");
    }
};

void setup()
{
    // put your setup code here, to run once:

    // monitor output for evaluation
    Serial.begin(115200);

    pinMode(CON_LED, OUTPUT);
    pinMode(HBUTTON_1, INPUT);
    pinMode(HBUTTON_2, INPUT);

    digitalWrite(CON_LED, LOW);

    // begin - https://api.laroomy.com/p/laroomy-api-class.html
    LaRoomyApi.begin();

    // set the bluetooth name
    LaRoomyApi.setBluetoothName("Esp32_Ble");

    // set the image for the device
    LaRoomyApi.setDeviceImage(LaRoomyImages::NOTIFICATION_TEXT_068);

    // set the callback handler for remote events
    LaRoomyApi.setCallbackInterface(
        dynamic_cast<ILaroomyAppCallback *>(
            new RemoteEvents()));

    // create the string interrogator
    StringInterrogator si;
    si.imageID = LaRoomyImages::STRING_REQUEST_166;
    si.stringInterrogatorDescription = "Login";
    si.stringInterrogatorID = SP_LOGIN_PROPERTY;

    // define the field attributes for the string-interrogator
    // https://api.laroomy.com/p/property-state-classes.html#laroomyApiRefMIDStringIS

    // set the text of the confirm-button
    si.stringInterrogatorState.buttonText = "Confirm and go back!";

    // set the attributes for field 1
    si.stringInterrogatorState.fieldOneDescriptor = "Username";
    si.stringInterrogatorState.fieldOneHint = "Enter username here";
    si.stringInterrogatorState.fieldOneInputType = StringInterrogatorFieldInputType::SI_INPUT_TEXT; // this is the default

    // set the attributes for field 2
    si.stringInterrogatorState.fieldTwoDescriptor = "Password";
    si.stringInterrogatorState.fieldTwoHint = "Enter password here";
    si.stringInterrogatorState.fieldTwoInputType = StringInterrogatorFieldInputType::SI_INPUT_TEXT_PASSWORD;

    // add it
    LaRoomyApi.addDeviceProperty(si);

    // create the text-list-presenter
    TextListPresenter tlp;
    tlp.imageID = LaRoomyImages::FORMAT_LIST_167;
    tlp.textListPresenterDescription = "Show Log data";
    tlp.textListPresenterID = SP_LOGDATA_PROPERTY;

    // add it
    LaRoomyApi.addDeviceProperty(tlp);

    // at last, call run to apply the setup and start bluetooth advertising
    LaRoomyApi.run();
}

void loop()
{
    // put your main code here, to run repeatedly:

    // first hardware button is pressed
    if (digitalRead(HBUTTON_1) == LOW)
    {
        delay(10);

        // add a message to the text-list-presenter with the method of the LaRoomyApi interface
        // https://api.laroomy.com/p/laroomy-api-class.html#laroomyApiRefMAddTLElement
        LaRoomyApi.addTextListPresenterElement(SP_LOGDATA_PROPERTY, TextListPresenterElementType::INFO_MESSAGE, "This is an info-type message.");

        while (digitalRead(HBUTTON_1) == LOW)
            ;
        delay(50);
    }

    // second hardware button is pressed
    if (digitalRead(HBUTTON_2) == LOW)
    {
        delay(10);

        // add a message to the text-list-presenter with the TextListPresenterClass itself
        // https://api.laroomy.com/p/property-classes.html#laroomyApiRefMIDTextListP
        TextListPresenter tlp(SP_LOGDATA_PROPERTY);
        tlp.addTextListPresenterElement(TextListPresenterElementType::ERROR_MESSAGE, "This is an error-type message.");

        // NOTE: calling the constructor of a specific property with the ID of a previously added property,
        //       the property is automatically retrieved. More info: https://api.laroomy.com/p/property-classes.html

        while (digitalRead(HBUTTON_2) == LOW)
            ;
        delay(50);
    }
}