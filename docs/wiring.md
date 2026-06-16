# Wiring

This project uses two I2C modules: **MPU6050** and **0.96-inch SSD1306 OLED**. Both modules share the same I2C pins.

## Arduino Nano / Uno wiring

| Module | Pin | Arduino connection |
|---|---|---|
| OLED | VCC | 5V |
| OLED | GND | GND |
| OLED | SDA | A4 / SDA |
| OLED | SCL | A5 / SCL |
| MPU6050 | VCC | 5V |
| MPU6050 | GND | GND |
| MPU6050 | SDA | A4 / SDA |
| MPU6050 | SCL | A5 / SCL |
| Buzzer | + | D11 |
| Buzzer | - | GND |
| Red LED | + | D12 through resistor |
| Red LED | - | GND |

## Notes

- The LED must be connected through a resistor. A value from **220 Ω to 1 kΩ** is suitable.
- If your OLED does not turn on, check whether the I2C address is `0x3C` or `0x3D`.
- If your MPU6050 does not respond, check VCC, GND, SDA, and SCL connections.
- On Arduino Uno/Nano, the I2C pins are normally **A4 = SDA** and **A5 = SCL**.
