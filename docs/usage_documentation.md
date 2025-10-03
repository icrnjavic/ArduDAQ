## Getting Started

### Serial Communication
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1
- **Flow Control**: None

### Supported commands
| Command           | Description                           | Response Format    |
|-------------------|---------------------------------------|--------------------|
| `*IDN?`          | Request device identification         | Device identifier  |
| `MEAS:VOLT:CHAN1?`| Read voltage from channel 1          | `<value>` in V     |
| `MEAS:VOLT:CHAN2?`| Read voltage from channel 2          | `<value>` in V     |
| `MEAS:VOLT:CHAN3?`| Read voltage from channel 3          | `<value>` in V     |
| `MEAS:VOLT:CHAN4?`| Read voltage from channel 4          | `<value>` in V     |
| `MEAS:CURR?`      | Read current measurement             | `<value>` in A     |
| `MEAS:TEMP?`      | Read temperature measurement         | `<value>` in °C|
| `OUTP:CHAN3 ON`   | Turn on channel 3 output             | Channel 3 ON       |
| `OUTP:CHAN3 OFF`  | Turn off channel 3 output            | Channel 3 OFF      |
| `OUTP:CHAN4 ON`   | Turn on channel 4 output             | Channel 4 ON       |
| `OUTP:CHAN4 OFF`  | Turn off channel 4 output            | Channel 4 OFF      |
| `OUTP:CHAN5 ON`   | Turn on channel 5 output             | Channel 5 ON       |
| `OUTP:CHAN5 OFF`  | Turn off channel 5 output            | Channel 5 OFF      |
| `OUTP:CHAN6 ON`   | Turn on channel 6 output             | Channel 6 ON       |
| `OUTP:CHAN6 OFF`  | Turn off channel 6 output            | Channel 6 OFF      |
| `OUTP:CHAN7 ON`   | Turn on channel 7 output             | Channel 7 ON       |
| `OUTP:CHAN7 OFF`  | Turn off channel 7 output            | Channel 7 OFF      |



### Example Usage

To read the voltage on Channel 1, send the command `MEAS:VOLT:CHAN1?` over serial. Board will respond with a message like: `3.3001V`



