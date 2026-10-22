# TMP102 Cheat Sheet


## 1. Fast Specs & Bench Requirements

* **Supply Voltage ($V+$)**: $1.4\,\text{V}$ to $3.6\,\text{V}$ (typically powered from the $3.3\,\text{V}$ rail).
* **Interface**: $\text{I}^2\text{C}$ / SMBus (Target / Slave only).
* **Default Conversion Rate**: $4\,\text{Hz}$ ($250\,\text{ms}$ conversion cycle, active conversion takes $\approx 10\,\text{ms}$).
* **Resolution**: $0.0625\,^{\circ}\text{C}$ per LSB ($12\text{-bit}$ default mode).
* **Measurement Range**: $-40\,^{\circ}\text{C}$ to $+125\,^{\circ}\text{C}$ (operates down to $-55\,^{\circ}\text{C}$).

---

## 2. Pinout & Breadboard Connections

```
        DRL (SOT563) Top View
             +---+--+---+
       SCL --| 1      6 |-- SDA
       GND --| 2      5 |-- V+ (3.3V)
     ALERT --| 3      4 |-- ADD0
             +----------+
```

| Pin | Name | Connect To | Hardware Notes |
|:---:|:----:|:-----------|:---------------|
| **1** | **SCL** | Host MCU $\text{I}^2\text{C}$ SCL | Needs pull-up resistor ($4.7\,\text{k}\Omega$ to $5\,\text{k}\Omega$) to $V+$. |
| **2** | **GND** | System Ground ($0\,\text{V}$) | Tie to host MCU common ground plane. |
| **3** | **ALERT**| Host MCU GPIO / Interrupt | Open-drain active-low output by default. Needs pull-up. Leave floating or pulled up if unused. |
| **4** | **ADD0** | GND, V+, SDA, or SCL | Hardware pin to select 7-bit bus address. |
| **5** | **V+** | $3.3\,\text{V}$ Power Rail | Place a $0.01\,\mu\text{F}$ to $0.1\,\mu\text{F}$ bypass cap right next to pin 2 & 5. |
| **6** | **SDA** | Host MCU $\text{I}^2\text{C}$ SDA | Needs pull-up resistor ($4.7\,\text{k}\Omega$ to $5\,\text{k}\Omega$) to $V+$. |

> **Hardware Rule of Thumb**: The TMP102 outputs are open-drain. Without external pull-ups on SDA and SCL, the bus will stay pinned at $0\,\text{V}$ and every transfer will time out or NACK.

---

## 3. $\text{I}^2\text{C}$ Addressing Cheat Sheet

The chip's 7-bit address depends on how pin 4 (`ADD0`) is tied:

| `ADD0` Tied To | 7-Bit Address (Hex) | 8-Bit Write Byte (`addr << 1`) | 8-Bit Read Byte (`(addr << 1) \| 1`) |
|:--------------:|:-------------------:|:------------------------------:|:-----------------------------------:|
| **GND**        | `0x48`              | `0x90`                         | `0x91`                              |
| **V+**         | `0x49`              | `0x92`                         | `0x93`                              |
| **SDA**        | `0x4A`              | `0x94`                         | `0x95`                              |
| **SCL**        | `0x4B`              | `0x96`                         | `0x97`                              |

* **SMBus Alert Response Address (ARA)**: `0x0C` (Used when polling who pulled the ALERT line).
* **Software Reset via General Call**: Send byte `0x00` (General Call Address), followed by byte `0x06`.

---

## 4. Register Pointers

Write one byte right after the $\text{I}^2\text{C}$ address to pick which register to read or update:

| Pointer Byte | Register | Size | Access | Reset Default | Description |
|:------------:|:---------|:----:|:------:|:-------------:|:------------|
| `0x00` | **Temperature** | 16-bit | Read-Only | `0x0000` | Most recent conversion result |
| `0x01` | **Configuration** | 16-bit | Read/Write | `0x60A0` | Operating modes, rates, fault queue |
| `0x02` | **$T_{\text{LOW}}$** | 16-bit | Read/Write | `0x4B00` ($+75\,^{\circ}\text{C}$) | Alert threshold lower limit |
| `0x03` | **$T_{\text{HIGH}}$** | 16-bit | Read/Write | `0x5000` ($+80\,^{\circ}\text{C}$) | Alert threshold upper limit |

---

## 5. Configuration Register Bit Breakdown (`0x01`)

The Configuration Register is 16 bits wide (2 bytes sent MSB first). Default is `0x60A0`.

```
Byte 1 (MSB):  [ OS | R1 | R0 | F1 | F0 | POL | TM | SD ]
               b15   b14  b13  b12  b11  b10   b9   b8

Byte 2 (LSB):  [ CR1 | CR0 | AL | EM |  0  |  0  |  0  |  0  ]
               b7    b6    b5   b4   b3   b2   b1   b0
```

### Key Flags & Settings

* **OS (One-Shot, Bit 15)**: Writing `1` in shutdown mode triggers a single $10\,\text{ms}$ conversion. Reads `0` while busy, `1` when complete.
* **F1/F0 (Fault Queue, Bits 12–11)**: Number of consecutive over-limit readings needed to fire ALERT:
  * `00` = 1 fault (instant)
  * `01` = 2 faults
  * `10` = 4 faults
  * `11` = 6 faults
* **POL (Alert Polarity, Bit 10)**: `0` = Active LOW (default), `1` = Active HIGH.
* **TM (Thermostat Mode, Bit 9)**: `0` = Comparator mode (stays tripped until temp $< T_{\text{LOW}}$), `1` = Interrupt mode (cleared by reading any register).
* **SD (Shutdown, Bit 8)**: `0` = Continuous sampling (draws $\approx 10\,\mu\text{A}$), `1` = Low-power sleep ($< 0.5\,\mu\text{A}$).
* **CR1/CR0 (Conversion Rate, Bits 7–6)**:
  * `00` = 0.25 Hz (every 4 s)
  * `01` = 1 Hz (every 1 s)
  * `10` = 4 Hz (default, every 250 ms)
  * `11` = 8 Hz (every 125 ms)
* **AL (Alert Bit, Bit 5)**: Read-only status. Reflects current ALERT pin state.
* **EM (Extended Mode, Bit 4)**: `0` = Standard 12-bit mode (up to $+128\,^{\circ}\text{C}$), `1` = 13-bit mode (up to $+150\,^{\circ}\text{C}$).

---

## 6. How the 12-Bit Temperature Math Works

### The Register Layout
The sensor returns 2 bytes for the temperature reading. The 12-bit reading is **left-justified**:

```
Byte 1 (MSB): [ D11 | D10 | D09 | D08 | D07 | D06 | D05 | D04 ]
Byte 2 (LSB): [ D03 | D02 | D01 | D00 |  0  |  0  |  0  |  0  ]
```

### The C Conversion Formula
1. Combine the two bytes into a signed 16-bit integer:
   $$\text{raw} = (\text{Byte 1} \ll 8) \mid \text{Byte 2}$$
2. Perform an **arithmetic right shift by 4** (this discards the 4 trailing zero bits while preserving negative sign bits):
   $$\text{counts} = (\text{int16\_t})\text{raw} \gg 4$$
3. Multiply by the step resolution ($0.0625\,^{\circ}\text{C}$):
   $$\text{temp\_c} = \text{counts} \times 0.0625\,\text{f}$$

```c
float tmp102_raw_to_celsius(uint16_t reg)
{
    /* Cast to signed int16_t BEFORE shifting so negative numbers sign-extend properly */
    int16_t count = (int16_t)reg >> 4;
    return (float)count * 0.0625f;
}
```

---

## 7. Bench Sanity Check Table (Datasheet Table 6-2)

Use this table when stepping through code in a debugger to verify your readings:

| Raw Hex (`reg`) | Arithmetic Shift (`>> 4`) | Celsius Equivalent | Test Case / Scenario |
|:---------------:|:-------------------------:|:------------------:|:---------------------|
| `0x7FF0`        | $+2047$                   | $+127.9375\,^{\circ}\text{C}$ | Maximum 12-bit positive reading |
| `0x5000`        | $+1280$                   | $+80.0\,^{\circ}\text{C}$     | Factory default $T_{\text{HIGH}}$ threshold |
| `0x4B00`        | $+1200$                   | $+75.0\,^{\circ}\text{C}$     | Factory default $T_{\text{LOW}}$ threshold |
| `0x1900`        | $+400$                    | $+25.0\,^{\circ}\text{C}$     | Room temperature |
| `0x0040`        | $+4$                      | $+0.25\,^{\circ}\text{C}$     | Small positive step |
| `0x0000`        | $0$                       | $0.0\,^{\circ}\text{C}$       | Freezing point |
| `0xFFC0`        | $-4$                      | $-0.25\,^{\circ}\text{C}$     | Small negative step |
| `0xE700`        | $-400$                    | $-25.0\,^{\circ}\text{C}$     | Standard freezer test |
| `0xC900`        | $-880$                    | $-55.0\,^{\circ}\text{C}$     | Minimum sensor measurement limit |

---

## 8. Common Bench Gotchas & Troubleshooting

1. **Negative temperatures reading as $+4000\,^{\circ}\text{C}$**:
   * *Cause*: Shifting an unsigned variable (`(uint16_t)reg >> 4`) inserts zeros at the top instead of preserving the sign bit.
   * *Fix*: Cast to `(int16_t)reg` first, then shift.
2. **Device always NACKs the address**:
   * *Cause*: Address format mismatch. Some MCU libraries require a 7-bit address (`0x48`), while others expect an 8-bit shifted write address (`0x90`).
3. **$\text{I}^2\text{C}$ Bus locked low**:
   * *Cause*: Missing pull-ups on SDA/SCL, or the MCU reset midway through a byte read.
   * *Fix*: Verify $4.7\,\text{k}\Omega$ resistors are present. If a bus hang occurs, clock 9 dummy pulses on SCL with SDA high to release the target device.
4. **First read always reads $0\,^{\circ}\text{C}$ right after power-on**:
   * *Cause*: Normal behavior. The sensor holds `0x0000` until its very first $10\,\text{ms}$ conversion cycle completes. Delay at least $15\,\text{ms}$ after power-up before taking the first reading.