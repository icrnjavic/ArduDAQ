# ArduDAQ

![Alt text](docs/pictures/1.png)

## Project Description

ArduDAQ is an open-source data acquisition shield for the popular Arduino platform with design to measure voltage, current, and onboard temperature with access to the rest of the pins from the base board. <br>
It's a fun side project to get more familiar and pratice HW and SW before starting a stand alone board. Component selection is somewhat sub optimal as i wanted to use components i had on hand.

## Features

- Multiple voltage measurement channels (16 bit ADC)
- Current measurement channel (10 bit ADC - for now)
- Onboard temperature sensing
- IO (PWM included)


## Testing

| Function         | Status          | Comments            |
|--------------------|-----------------|---------------------|
| Voltage Channels   | ✅ Tested        | Initial test seems good. |
| Current Channel    | ✅ Tested        | Works relatively well but accuracy for super low currents not the best due to use of internal adc which is only 10bit.|
| Onboard Temperature| ✅ Tested  | Initital test seems good.  |
| IO | ❌ Not Tested  |  SW support in WIP stake |
| Desktop APP | 🚧  WIP  |  |

Above tests were just the basic tests on the bench.
More detailed testing will be done after the test jig is finished.

Too test the Voltage channel accuracy i teste accuracy so that I measured voltage on the PSU where i was incrementing the ouptu by 0.5V. For a reference point i was also measuring the output with a OWON XD1041 multimeter.
![Alt text](docs/pictures/accuracy_test.png)
As seen above on the cannel i was testing the accuracy was pretty good, with a max deviation of 8mV @ 50V compared to the multimeter.


## Dekstop App
Alongside ArduDAQ shield's support for serial commands for automated measurements it also has a desktop app(***still WIP***).<br>
It will be able to put the ArduDAQ in continuous mode to display measured voltages on selected voltage channels over time and plot those measurements.
![Alt text](docs/pictures/desktop_app.png)


## Getting Started

### Serial Communication
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1
- **Flow Control**: None

### Supported commands
| Command | Description | Response Format |
|---------|-------------|-----------------|
| `READ_CHANNEL_1` | Read Voltage Channel 1 | `<value>` in V |
| `READ_CHANNEL_2` | Read Voltage Channel 2 | `<value>` in V |
| `READ_CHANNEL_3` | Read Voltage Channel 3 | `<value>` in V |
| `READ_CHANNEL_4` | Read Voltage Channel 4 | `<value>` in V |
| `READ_CURRENT` | Read current draw | `<value>` in A |
| `READ_TEMPERATURE` | Read Onboard Temperature | `<value>` in °C|

### Example Usage

To read the voltage on Channel 1, send the command `READ_CHANNEL_0` over serial. Board will respond with a message like: `3.30V`



