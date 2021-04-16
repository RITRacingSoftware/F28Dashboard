#include <float.h>
#include "tft-processor.h"
#include "../../include/display-interfaces/display-item-intf.h"
#include "../tft-display/rectangle-item/tft-rectangle-item.h"
#include "../tft-display/text-item/tft-text-item.h"
#include "../tft-display/tft-display.h"
#include "etl/cstring.h"
#include "Arduino.h"
#include "../controller/dash-controller.h"
#include "../f29bms_dbc.h"


#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

void TFT_PROCESSOR::updateScreen()
{
    //Serial.println("thisUpdate");
    myDisplay.updateScreen();
}

TFT_PROCESSOR::TFT_PROCESSOR(DASH_CONTROLLER_INTF *dashController) : myDashController(dashController),
                                                                     myDisplay(10, 9),
                                                                     //Element(TFT_TEXT_ITEM(font_size, xCoordinate, yCoordinate, foreColor, BackgroundColor, text), TFT_RECTANGLE_ITEM(xCoordinate, yCoordinate, width, height, color))
                                                                     motorControllerFaults(TFT_TEXT_ITEM(0, 0, 180, RA8875_WHITE, RA8875_RED, "Motor Controller Faults: "), TFT_RECTANGLE_ITEM(0, 180, 600, 50, RA8875_BLACK)),
                                                                     motorSpeed(TFT_TEXT_ITEM(0, 0, 40, RA8875_WHITE, RA8875_RED, "Motor Speed: 000"), TFT_RECTANGLE_ITEM(0, 40, 150, 20, RA8875_BLACK)),
                                                                     //busVoltage(TFT_TEXT_ITEM(0, 300, 0, RA8875_WHITE, RA8875_RED, "Bus Voltage = 000"), TFT_RECTANGLE_ITEM(300, 0, 250, 20, RA8875_BLACK)),
                                                                     //outputVoltage(TFT_TEXT_ITEM(0, 0, 30, RA8875_WHITE, RA8875_RED, "Output Voltage = 000"), TFT_RECTANGLE_ITEM(0, 30, 140, 20, RA8875_BLACK)),
                                                                     //maxTemp(TFT_TEXT_ITEM(1, 250, 60, RA8875_WHITE, RA8875_RED, "Tmax: 000"), TFT_RECTANGLE_ITEM(250, 60, 150, 40, RA8875_BLACK)),
                                                                     //packVoltage(TFT_TEXT_ITEM(1, 25, 225, RA8875_WHITE, RA8875_RED, "Vtotal: 000"), TFT_RECTANGLE_ITEM(25, 225, 60, 40, RA8875_BLACK)),
                                                                     //batteryPercentage(TFT_TEXT_ITEM(1, 0, 60, RA8875_WHITE, RA8875_RED, "Battery% = 100"), TFT_RECTANGLE_ITEM(0, 60, 225, 40, RA8875_BLACK)),
                                                                     lapNumber(TFT_TEXT_ITEM(1, 250, 60, RA8875_WHITE, RA8875_RED, "Lap: 0"), TFT_RECTANGLE_ITEM(250, 60, 50, 45, RA8875_BLACK)),
                                                                     batteryPerLap(TFT_TEXT_ITEM(2, 0, 0, RA8875_WHITE, RA8875_RED, "Bat/Lap: 0"), TFT_RECTANGLE_ITEM(200, 0, 125, 50, RA8875_BLACK)),
                                                                     waterTemp(TFT_TEXT_ITEM(0, 150, 200, RA8875_WHITE, RA8875_RED, "Twater: 0"), TFT_RECTANGLE_ITEM(150, 200, 175, 20, RA8875_BLACK)),
                                                                     BMSFaults(TFT_TEXT_ITEM(0, 0, 130, RA8875_WHITE, RA8875_RED, "BMS Faults: "), TFT_RECTANGLE_ITEM(0, 130, 480, 50, RA8875_BLACK)),
                                                                     BMSFaultVector(TFT_TEXT_ITEM(0, 0, 110, RA8875_WHITE, RA8875_RED, "BMS Fault Vector: "), TFT_RECTANGLE_ITEM(0, 110, 280, 50, RA8875_BLACK)),
                                                                     ReadyToDriveStatus(TFT_TEXT_ITEM(1, 0, 230, RA8875_RED, RA8875_RED, "NOT READY TO DRIVE"), TFT_RECTANGLE_ITEM(0, 230, 300, 36, RA8875_BLACK)),
                                                                     MotorSpeedBar(5, 5, 0, 30, RA8875_GREEN),
                                                                     BMSMaxCurrent(TFT_TEXT_ITEM(0, 0, 65, RA8875_WHITE, RA8875_RED, "Max Current: "), TFT_RECTANGLE_ITEM(0, 65, 160, 20, RA8875_BLACK)),
                                                                     BMSMinVoltage(TFT_TEXT_ITEM(0, 317, 40, RA8875_WHITE, RA8875_RED, "Min Voltage: "), TFT_RECTANGLE_ITEM(317, 40, 160, 25, RA8875_BLACK)),
                                                                     BMSMaxVoltage(TFT_TEXT_ITEM(0, 317, 90, RA8875_WHITE, RA8875_RED, "Max Voltage: "), TFT_RECTANGLE_ITEM(317, 90, 160, 25, RA8875_BLACK)),
                                                                     BMSCurrentCurrent(TFT_TEXT_ITEM(0, 32, 85, RA8875_WHITE, RA8875_RED, "Current: "), TFT_RECTANGLE_ITEM(32, 85, 130, 25, RA8875_BLACK)),
                                                                     BMSSOC(TFT_TEXT_ITEM(0, 205, 65, RA8875_WHITE, RA8875_RED, "SOC: "), TFT_RECTANGLE_ITEM(205, 65, 100, 20, RA8875_BLACK)),
                                                                     BMSSOCRaw(TFT_TEXT_ITEM(0, 165, 40, RA8875_WHITE, RA8875_RED, "SOC(raw): "), TFT_RECTANGLE_ITEM(165, 40, 100, 25, RA8875_BLACK)),
                                                                     BMSPackVoltage(TFT_TEXT_ITEM(0, 310, 65, RA8875_WHITE, RA8875_RED, "Pack Voltage: "), TFT_RECTANGLE_ITEM(310, 65, 160, 25, RA8875_BLACK))
{
    this->lap = 0;
    this->batteryBeforeLap = 100;
    //this->batteryPercent = 100;
    this->previoustMCFaultString = "Motor Controller Faults: ";
    this->previousBMSFaultString = "BMS Faults: ";
    this->prevFaultVector = 0x0;
    this->minVoltage = DBL_MAX;
    this->maxCurrent = DBL_MIN;
    this->maxVoltage = DBL_MIN;
    this->SOC = 0;
    this->SOCRaw = 0;
    this->packVoltage = 0;
    char testString[MAX_STRING_SIZE];
    sprintf(testString, "Max Current: %f", 5.5);
    this->SOCRaw = 69;
    char SOCstring[MAX_STRING_SIZE];
    BMSMaxCurrent.updateText(testString);
    sprintf(SOCstring, "SOC(raw): %.1f", SOCRaw);
    BMSSOCRaw.updateText(SOCstring);

}

void TFT_PROCESSOR::initializeCallbacks()
{
    //create callbacks and then register them
    //Create elements and send pointer to addElemnt to register it

    //this->myDisplay.addElement(&maxTemp);
    this->myDisplay.addElement(&ReadyToDriveStatus);
    this->myDisplay.addElement(&motorControllerFaults);
    this->myDisplay.addElement(&motorSpeed);
    //this->myDisplay.addElement(&batteryPercentage);
    //this->myDisplay.addElement(&BMSFaults);

    this->myDisplay.addElement(&MotorSpeedBar);
    //this->myDisplay.addElement(&BlackMotorSpeedBar);

    //New BMS stuff:
    this->myDisplay.addElement(&BMSFaults);
    this->myDisplay.addElement(&BMSFaultVector);
    this->myDisplay.addElement(&BMSMaxCurrent);
    this->myDisplay.addElement(&BMSMinVoltage);
    this->myDisplay.addElement(&BMSMaxVoltage);
    this->myDisplay.addElement(&BMSCurrentCurrent);
    this->myDisplay.addElement(&BMSSOC);
    this->myDisplay.addElement(&BMSSOCRaw);
    this->myDisplay.addElement(&BMSPackVoltage);

    //Register callbacks. Callbacks must be registered in DASH_CONTROLLER::registerCallbacks
    //for callbacks to be called
    myDashController->registerCallback();
}

void TFT_PROCESSOR::updateMCFaultText(etl::array<uint8_t, 8> const &data)
{
    char faultsString[MAX_STRING_SIZE] = "Motor Controller Faults: ";

    //Use each byte of CAN data and List of fault messages to check all of the motor controller faults
    checkFaults(data[0], MCByteZero, faultsString);
    checkFaults(data[1], MCByteOne, faultsString);
    checkFaults(data[2], MCByteTwo, faultsString);
    checkFaults(data[3], MCByteThree, faultsString);
    checkFaults(data[4], MCByteFour, faultsString);
    checkFaults(data[5], MCByteFive, faultsString);
    checkFaults(data[6], MCByteSix, faultsString);
    checkFaults(data[7], MCByteSeven, faultsString);

    //Only update string if faults have changed
    //if (strcmp(faultsString, previoustMCFaultString) != 0)
    //{
    motorControllerFaults.updateText(faultsString);
    //}
    previoustMCFaultString = faultsString;

    //If accumulator temp is 16 bit value, send 16 bits ntoh function to corrtect endianess
}

void TFT_PROCESSOR::MotorPositionInformation(etl::array<uint8_t, 8> const &data)
{
    char motorSpeedNum[MAX_STRING_SIZE];
    uint16_t number = data[2] | (data[3] << 8);
    if (data[0] == 0)
    {
        //motorSpeedRect.updateColor(RA8875_RED);
    }
    sprintf(motorSpeedNum, "Motor Speed: %d", number);
    Serial.println(motorSpeedNum);
    motorSpeed.updateText(motorSpeedNum);
    uint16_t barWidth = (uint16_t)((number / 4000.0) * 470.0);
    //MotorSpeedBar.updateRectangleSize(barWidth, 30);
    Serial.printf("Motor speed bar width: %d\n\r", barWidth);
    MotorSpeedBar.updateSize(barWidth, 30);
    //MotorSpeedBar.updateTextLocation(barWidth, 5);
}

// void TFT_PROCESSOR::VoltageInfo(etl::array<uint8_t, 8> const &data)
// {
//     char outputVoltageNum[MAX_STRING_SIZE];
//     uint16_t number = data[2] | (data[3] << 8);
//     sprintf(outputVoltageNum, "Output Voltage = %d", number);
//     outputVoltage.updateText(outputVoltageNum);

//     char busVoltageNum[MAX_STRING_SIZE];
//     number = data[0] | (data[1] << 8);
//     sprintf(busVoltageNum, "Bus Voltage = %d", number);
//     busVoltage.updateText(busVoltageNum);
// }

// void TFT_PROCESSOR::AccumTemp(etl::array<uint8_t, 8> const &data)
// {
//     char maxTempNum[MAX_STRING_SIZE];
//     uint16_t number = data[4];
//     sprintf(maxTempNum, "Tmax: %d", number);
//     if (number > 50)
//     {
//         maxTemp.updateTextColor(RA8875_RED);
//     }
//     else if (number > 40)
//     {
//         maxTemp.updateTextColor(RA8875_YELLOW);
//     }
//     else
//     {
//         maxTemp.updateTextColor(RA8875_WHITE);
//     }
    
//     maxTemp.updateText(maxTempNum);
// }

// void TFT_PROCESSOR::AccumVoltage(etl::array<uint8_t, 8> const &data)
// {
//     char packVoltageNum[MAX_STRING_SIZE];
//     uint16_t number = data[0] | (data[1] << 8);
//     sprintf(packVoltageNum, "Vtotal: %d", number);
//     //packVoltage.updateText(packVoltageNum);
// }

// void TFT_PROCESSOR::AccumCharge(etl::array<uint8_t, 8> const &data)
// {
//     char batteryPercentNum[MAX_STRING_SIZE];
//     uint16_t number = data[0];
//     batteryPercent = number; //Update current battery percentage for battery/lap
//     sprintf(batteryPercentNum, "Battery  = %d", number);
//     Serial.println(batteryPercentNum);
//     batteryPercentage.updateText(batteryPercentNum);
// }

void TFT_PROCESSOR::IncrementLap(etl::array<uint8_t, 8> const &data)
{
    lap += 1;
    char lapNum[MAX_STRING_SIZE];
    uint16_t number = lap;
    sprintf(lapNum, "Lap: %d", number);
    lapNumber.updateText(lapNum);

    //Determine battery used last lap
    char batPerLapNum[MAX_STRING_SIZE];
    int difference = packVoltage - batteryBeforeLap; //Calulate battery used
    Serial.print("Differnce");
    Serial.println(number);
    Serial.print("Percent");
    //Serial.println(batteryPercent);
    Serial.print("old");
    Serial.println(batteryBeforeLap);
    sprintf(batPerLapNum, "Bat/Lap: %d", difference);
    batteryPerLap.updateText(batPerLapNum);
    //batteryBeforeLap = batteryPercent; //Set new starting percentage
}

void TFT_PROCESSOR::waterTempInfo(etl::array<uint8_t, 8> const &data)
{
    char waterTempNum[MAX_STRING_SIZE];
    uint16_t number = data[0]; //Change
    sprintf(waterTempNum, "Twater: %d", number);
    waterTemp.updateText(waterTempNum);
}

void TFT_PROCESSOR::readyToDriveMessage(etl::array<uint8_t, 8> const &data)
{
    uint16_t vehicleState = ((data[1] << 8) | data[0]);
    Serial.printf("Received State message, state is %d\n\r", vehicleState);
    if (vehicleState == 4)
    {
        this->myDashController->readyToDrive();
        ReadyToDriveStatus.updateText("READY TO DRIVE");
        ReadyToDriveStatus.updateTextColor(RA8875_GREEN);
    }
    else if (vehicleState == 5 || vehicleState == 6)
    {
        ReadyToDriveStatus.updateText("MOTOR POWERED");
    }
    else
    {
        ReadyToDriveStatus.updateText("NOT READY TO DRIVE");
        ReadyToDriveStatus.updateTextColor(RA8875_RED);
    }
}

boolean isFault(uint8_t status, uint8_t mask)
{
    return (status && mask) != 0;
}

// void TFT_PROCESSOR::updateBMSFaults(etl::array<uint8_t, 8> const &data)
// {
//     char BMSFaultsString[MAX_STRING_SIZE] = "BMS Faults: ";

//     checkFaults(data[0], stateOfSystem, BMSFaultsString);
//     //Don't know how to decode Fault Codes byte
//     checkFaults(data[5], faultFlags, BMSFaultsString);
//     Serial.println(BMSFaultsString);
//     BMSFaults.updateText(BMSFaultsString);
//     //Didn't add warnings
// }

//Takes a byte of data and a list of 8 messages, and if a bit is set adds the corresponding message to the fault list
void TFT_PROCESSOR::checkFaults(uint8_t data, etl::array<char[MAX_STRING_SIZE], 8> messages, char faultOutString[MAX_STRING_SIZE])
{
    //Only check each bit if one is set
    if (data != 0)
    {
        for (int i = 0; i < 8; i++)
        {
            if ((data & (0x1 << i)) > 0)
            {
                //char msg[MAX_STRING_SIZE];
                //snprintf(msg, MAX_STRING_SIZE, "Fault for index %d of msg %d\n\r", i, data);
                strncat(faultOutString, messages[i], MAX_STRING_SIZE - strlen(faultOutString));
                strncat(faultOutString, ", ", MAX_STRING_SIZE - strlen(faultOutString));
                //Serial.println(faults);
            }
        }
    }
}

void TFT_PROCESSOR::bmsFaults(etl::array<uint8_t, 8> const &data)
{
    f29bms_dbc_bms_fault_vector_unpack(&canBus.bms_fault_vector, (uint8_t*) data[0], 8);
    uint64_t incomingFaults = 0;
    for(int i = 0; i < 8; i++){
        incomingFaults |= (data[i] << (i*8));
    }
    if(incomingFaults != 0){
        BMSFaults.updateRectangleColor(RA8875_RED);
    }
    else if(incomingFaults != this->prevFaultVector){
        BMSFaults.updateRectangleColor(RA8875_BLACK);
    }
    if(incomingFaults != this->prevFaultVector){
        char BMSFaultVectorString[MAX_STRING_SIZE];
        sprintf(BMSFaultVectorString, "BMS Fault Vector: "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(data[1]), BYTE_TO_BINARY(data[0]));
        BMSFaultVector.updateText(BMSFaultVectorString);
        

        char BMSFaultsString[MAX_STRING_SIZE] = "BMS Faults: ";

        Serial.println("123");
        Serial.printf("data 0 = %d", data[0]);

        if(CHECK_BIT(data[0], 0)){
            Serial.print("Here");
            strncat(BMSFaultsString, "slave comm cells, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        }
        if(CHECK_BIT(data[0], 1)){
            strncat(BMSFaultsString, "slave comm temps, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        }
        if(CHECK_BIT(data[0], 2)){
            strncat(BMSFaultsString, "slave comm drain request, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        }
        if(CHECK_BIT(data[0], 3)){
            strncat(BMSFaultsString, "current sensor comm, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        }
        if(CHECK_BIT(data[0], 4)){
            strncat(BMSFaultsString, "current, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        }
        if(CHECK_BIT(data[0], 5)){
            strncat(BMSFaultsString, "cell voltage irrational, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        }
        if(CHECK_BIT(data[0], 6)){
            strncat(BMSFaultsString, "cell voltage diff, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        }
        if(CHECK_BIT(data[0], 7)){
            strncat(BMSFaultsString, "out of juice, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        }
        if(CHECK_BIT(data[1], 0)){
            strncat(BMSFaultsString, "temperature irrational, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        }
        if(CHECK_BIT(data[1], 1)){
            strncat(BMSFaultsString, "over temperature, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        }
        if(CHECK_BIT(data[1], 2)){
            strncat(BMSFaultsString, "drain failure, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        }

        // if(!f29bms_dbc_bms_fault_vector_bms_fault_vector_slave_comm_cells_is_in_range(canBus.bms_fault_vector.bms_fault_vector_slave_comm_cells)){
        //     Serial.print("Here");
        //     strncat(BMSFaultsString, "slave comm cells not in range, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        // }
        // if(!f29bms_dbc_bms_fault_vector_bms_fault_vector_slave_comm_temps_is_in_range(canBus.bms_fault_vector.bms_fault_vector_slave_comm_temps)){
        //     strncat(BMSFaultsString, "slave comm cells not in range, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        // }
        // if(!f29bms_dbc_bms_fault_vector_bms_fault_vector_slave_comm_drain_request_is_in_range(canBus.bms_fault_vector.bms_fault_vector_slave_comm_drain_request)){
        //     strncat(BMSFaultsString, "slave comm drain request not in range, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        // }
        // if(!f29bms_dbc_bms_fault_vector_bms_fault_vector_current_sensor_comm_is_in_range(canBus.bms_fault_vector.bms_fault_vector_current_sensor_comm)){
        //     strncat(BMSFaultsString, "current sensor comm not in range, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        // }
        // if(!f29bms_dbc_bms_fault_vector_bms_fault_vector_over_current_is_in_range(canBus.bms_fault_vector.bms_fault_vector_over_current)){
        //     strncat(BMSFaultsString, "current not in range, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        // }
        // if(!f29bms_dbc_bms_fault_vector_bms_fault_vector_cell_voltage_irrational_is_in_range(canBus.bms_fault_vector.bms_fault_vector_cell_voltage_irrational)){
        //     strncat(BMSFaultsString, "cell voltage irrational, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        // }
        // if(!f29bms_dbc_bms_fault_vector_bms_fault_vector_cell_voltage_diff_is_in_range(canBus.bms_fault_vector.bms_fault_vector_cell_voltage_diff)){
        //     strncat(BMSFaultsString, "cell voltage diff not in range, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        // }
        // if(!f29bms_dbc_bms_fault_vector_bms_fault_vector_out_of_juice_is_in_range(canBus.bms_fault_vector.bms_fault_vector_out_of_juice)){
        //     strncat(BMSFaultsString, "out of juice, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        // }
        // if(!f29bms_dbc_bms_fault_vector_bms_fault_vector_temperature_irrational_is_in_range(canBus.bms_fault_vector.bms_fault_vector_temperature_irrational)){
        //     strncat(BMSFaultsString, "temperature irrational, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        // }
        // if(!f29bms_dbc_bms_fault_vector_bms_fault_vector_over_temperature_is_in_range(canBus.bms_fault_vector.bms_fault_vector_over_temperature)){
        //     strncat(BMSFaultsString, "over temperature, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        // }
        // if(!f29bms_dbc_bms_fault_vector_bms_fault_vector_drain_failure_is_in_range(canBus.bms_fault_vector.bms_fault_vector_drain_failure)){
        //     strncat(BMSFaultsString, "drain failure, ", MAX_STRING_SIZE - strlen(BMSFaultsString));
        // }
        
        this->prevFaultVector = incomingFaults;
        BMSFaults.updateText(BMSFaultsString);
    }
}

void TFT_PROCESSOR::bmsCurrent(etl::array<uint8_t, 8> const &data)
{
    f29bms_dbc_bms_current_unpack(&canBus.bms_current, (uint8_t*) data[0], 8);
    double curCurrent = f29bms_dbc_bms_current_bms_inst_current_filt_decode(canBus.bms_current.bms_inst_current_filt);
    if(curCurrent > this->maxCurrent){
        this->maxCurrent = curCurrent;
        char maxCurrentString[MAX_STRING_SIZE];
        sprintf(maxCurrentString, "Max Current: %f", this->maxCurrent);
        BMSMaxCurrent.updateText(maxCurrentString);
    }
    char currentString[MAX_STRING_SIZE];
    sprintf(currentString, "Current: %f", curCurrent);
    BMSCurrentCurrent.updateText(currentString);
}

void TFT_PROCESSOR::bmsVoltages(etl::array<uint8_t, 8> const &data)
{
    f29bms_dbc_bms_voltages_unpack(&canBus.bms_voltages, (uint8_t*) data[0], 8);
    //check if any just got are lower, if so, update
    uint64_t rawData;
    for(int i = 0; i < 8; i++){
        rawData |= (data.at(i) << (i*8));
    }

    uint16_t mask = 0x1FF;
    for(int i = 0; i < 6; i ++){
        uint16_t thisRawVoltage = (rawData >> (8 + (9*i))) && mask;
        double thisVoltage = f29bms_dbc_bms_voltages_bms_voltages_cell0_decode(thisRawVoltage);
        if(thisVoltage < this->minVoltage){
            this->minVoltage = thisVoltage;
            char minVoltageString[MAX_STRING_SIZE];
            sprintf(minVoltageString, "Min Voltage: %f", this->minVoltage);
            BMSMinVoltage.updateText(minVoltageString);
        }
        //Do we want to change this to be max voltage of cells right now, instead of just the max voltage seen all time?
        if(thisVoltage > this->maxVoltage){
            this->maxVoltage = thisVoltage;
            char maxVoltageString[MAX_STRING_SIZE];
            sprintf(maxVoltageString, "Max Voltage: %f", this->maxVoltage);
            BMSMaxVoltage.updateText(maxVoltageString);
        }
    }
}

void TFT_PROCESSOR::bmsStatus(etl::array<uint8_t, 8> const &data)
{
    f29bms_dbc_bms_status_unpack(&canBus.bms_status, (uint8_t*) data[0], 8);
    int thisSOC = data[0];
    if(thisSOC != SOC){
        this->SOC = thisSOC;
        char SOCstring[MAX_STRING_SIZE];
        sprintf(SOCstring, "SOC: %d", this->SOC);
        BMSSOC.updateText(SOCstring);
    }
    int thisSOCRaw = data[1];
    if(thisSOCRaw != SOCRaw){
        this->SOCRaw = thisSOCRaw;
        char SOCstring[MAX_STRING_SIZE];
        sprintf(SOCstring, "SOC(raw): %d", this->SOCRaw);
        BMSSOCRaw.updateText(SOCstring);
    }

    uint16_t rawpackvoltage = (data[2] << 8) | data[3];
    Serial.printf("Recieved = %u\n", rawpackvoltage);
    double thisPackVoltage = f29bms_dbc_bms_status_bms_status_pack_voltage_decode(rawpackvoltage);//(((data[2] & 0x07) << 7) | ((data[3] & 0xF7) >> 1));
    thisPackVoltage += 0.1;
    if(thisPackVoltage != packVoltage){
        this->packVoltage = thisPackVoltage;
        char voltstring[MAX_STRING_SIZE];
        sprintf(voltstring, "Pack Voltage: %.3f", packVoltage);
        BMSPackVoltage.updateText(voltstring);
    }
}

void TFT_PROCESSOR::clearScreen()
{
    this->myDisplay.clearScreen();
}

void TFT_PROCESSOR::test(){
    etl::array<uint8_t, 8> data;
    data[0] = 69;
    data[1] = 12;
    data[2] = 1;
    data[3] = (35 << 1);
    data[4] = 0;
    data[5] = 0;
    data[6] = 0;
    data[7] = 0;
    uint16_t pack = f29bms_dbc_bms_status_bms_status_pack_voltage_encode(78.5);
    Serial.printf("pack = %u\n", pack);
    data[2] = (uint8_t) ((pack >> 8) & 0x7);
    Serial.printf("data[2] = %u\n", data[2]);
    data[3] = (uint8_t) (pack & 0xFF);
    Serial.printf("data[3] = %u\n", data[3]);
    bmsStatus(data);
    for(int i  = 0; i < 8; i++){
        data[i] = 0x00;
    }
    data[1] = 0x01;
    
    //data[1] = 1;
    bmsFaults(data);
    // delay(10000);
    // data[1] = 0;
    // bmsFaults(data);
    this->myDashController->updateModel();
    this->myDashController->updateView();
}