# ArduDAQ

## Project Description

ArduDAQ is an open-source data acquisition shield for the popular Arduino platform with design to measure voltage, current, and onboard temperature with access to the rest of the  using Arduino hardware. <br>
It's a fun side project with cost-effectivenes and customizability in mind for automating basic measurements.

## Features

- Multiple voltage measurement channels (16 bit ADC)
- Current measurement channel
- Onboard temperature sensing
- IO (PWM included)


## Testing

| Function         | Status          | Comments            |
|--------------------|-----------------|---------------------|
| Voltage Channels   | ✅ Tested        | Initial test looks good. |
| Current Channel    | ✅ Tested        | After some tweaks readings look good but not too acurate under the 40mA range. |
| Onboard Temperature| 🚧 In Progress  |   |
| IO | ❌ Not Tested  |  Not yet supported in the SW. |

Above tests were just the basic tests on the bench.
More detailed testing will be done after I finish the test jig.



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
| `READ_CHANNEL_0` | Read Voltage Channel 1 | `<value>` in V |
| `READ_CHANNEL_1` | Read Voltage Channel 2 | `<value>` in V |
| `READ_CHANNEL_2` | Read Voltage Channel 3 | `<value>` in V |
| `READ_CHANNEL_3` | Read Voltage Channel 4 | `<value>` in V |
| `READ_CURRENT` | Read current draw | `<value>` in A |
| `READ_TEMPERATURE` | Read Onboard Temperature | `<value>` in °C|

### Example Usage

To read the voltage on Channel 1, send the command `READ_CHANNEL_0` over serial. Board will respond with a message like: `3.30V`

## License

MIT License


