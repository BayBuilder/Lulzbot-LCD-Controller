// ===== CYD CONFIGURATION =====
//
//  Pin Name    CYD use     Phys pin#   Function (I=input, O=output, P=power), Alternate Functions
//  GND         Power       1                       Ground
//  V_IN        Power       2                       3.3V
//  EN          #RESET      3           I           Enable / Disable ESP-32 module
//  SENSOR_VP   Touch       4           I   GPIO36, ADC1_CH0, RTC_GPIO0
//  SENSOR_VN   Touch       5           I   GPIO39, ADC1_CH3, RTC_GPIO3
//  IO34        LightSense  6           I   GPIO34, ADC1_CH6, RTC_GPIO4
//  IO35                    7           I   GPIO35, ADC1_CH7, RTC_GPIO5
//  IO32        Touch       8           I/O GPIO32, XTAL_32K_P (32.768 kHz Xtal in), ADC1_CH4, TOUCH9, RTC_GPIO9
//  IO33        Touch       9           I/O GPIO33, XTAL_32K_N (32.768 kHz Xtal out), ADC1_CH5, TOUCH8, RTC_GPIO8
//  IO25        Touch       10          I/O GPIO25, DAC_1, ADC2_CH8, RTC_GPIO6, EMAC_RXD0
//  IO26        Speaker     11          I/O GPIO26, DAC_2, ADC2_CH9, RTC_GPIO7, EMAC_RXD1
//  IO27                    12          I/O GPIO27, ADC2_CH7, TOUCH7, RTC_GPIO17, EMAC_RX_DV
//  IO14        LCD         13          I/O GPIO14, ADC2_CH6, TOUCH6, RTC_GPIO16, MTMS, HSPICLK, HS2_CLK, SD_CLK, EMAC_TXD2
//  IO12        LCD         14          I/O GPIO12, ADC2_CH5, TOUCH5, RTC_GPIO15, MTDI, HSPIQ, HS2_DATA2, SD_DATA2, EMAC_TXD3
//  GND         Power       15          P           Ground
//  IO13        LCD         16          I/O GPIO13, ADC2_CH4, TOUCH4, RTC_GPIO14, MTCK, HSPID, HS2_DATA3, SD_DATA3, EMAC_RX_ER
//  SHD/SD2     Flash ROM   17          I/O GPIO9,  SD_DATA2, SPIHD, HS1_DATA2, U1RXD
//  SWP/SD3     Flash ROM   18          I/O GPIO10, SD_DATA3, SPIWP, HS1_DATA3, U1TXD
//  SCS/CMD     Flash ROM   19          I/O GPIO11, SD_CMD, SPICS0, HS1_CMD, U1RTS
//  SCK/CLK     Flash ROM   20          I/O GPIO6,  SD_CLK, SPICLK, HS1_CLK, U1CTS
//  SDO/SD0     Flash ROM   21          I/O GPIO7,  SD_DATA0, SPIQ, HS1_DATA0, U2RTS
//  SDI/SD1     Flash ROM   22          I/O GPIO8,  SD_DATA1, SPID, HS1_DATA1, U2CTS
//  IO15        LCD         23          I/O GPIO15, ADC2_CH3, TOUCH3, MTDO, HSPICS0, RTC_GPIO13, HS2_CMD, SD_CMD, EMAC_RXD3
//  IO2         LCD         24          I/O GPIO2,  ADC2_CH2, TOUCH2, RTC_GPIO12, HSPIWP, HS2_DATA0, SD_DATA0
//  IO0         Boot SW     25          I/O GPIO0,  ADC2_CH1, TOUCH1, RTC_GPIO11, CLK_OUT1, EMAC_TX_CLK
//  IO4         LED_R       26          I/O GPIO4,  ADC2_CH0, TOUCH0, RTC_GPIO10, HSPIHD, HS2_DATA1, SD_DATA1, EMAC_TX_ER
//  IO16       *LED_G       27          I/O GPIO16, HS1_DATA4, U2RXD, EMAC_CLK_OUT
//  IO17       *LED_B       28          I/O GPIO17, HS1_DATA5, U2TXD, EMAC_CLK_OUT_180
//  IO5         SD Card     29          I/O GPIO5,  VSPICS0, HS1_DATA6, EMAC_RX_CLK
//  IO18        SD Card     30          I/O GPIO18, VSPICLK, HS1_DATA7
//  IO19        SD Card     31          I/O GPIO19, VSPIQ, U0CTS, EMAC_TXD0
//  NC          NC          32                      Not Connected
//  IO21        Backlight   33          I/O GPIO21, VSPIHD, EMAC_TX_EN
//  RXD0        U0RXD       34          I/O GPIO3,  U0RXD, CLK_OUT2
//  TXD0        U0TXD       35          I/O GPIO1,  U0TXD, CLK_OUT3, EMAC_RXD2
//  IO22                    36          I/O GPIO22, VSPIWP, U0RTS, EMAC_TXD1
//  IO23        SD Card     37          I/O GPIO23, VSPID, HS1_STROBE
//  GND         Power       38          P           Ground
//
//  * = Repurpose for serial communications w/ Lulzbot Mini
//      Possible alternative: Use available GPIO to signal when it's ok to switch baud rates and
//      communicate with the printer using UART0. Add voltage level shifters as needed.
//
//      GPIO22 and GPIO27 are free for general purpose I/O
//      GPIO35 is available as INPUT only
//
// XPT2046 Touchscreen Driver
//      GPIO25      T_CLK
//      GPIO32      T_MOSI
//      GPIO33      T_CS
//      GPIO36      T_IRQ
//      GPIO39      T_MISO
//
// ST7789 LCD Driver
//      GPIO2       TFT_DC data/command
//      GPIO12      TFT_MISO
//      GPIO13      TFT_MOSI
//      GPIO14      TFT_SCK
//      GPIO15      TFT_CS
//
// LCD Backlight      
//      GPIO21      backlight PWM (brightness)

// ===== SD CARD CONFIGURATION =====
//      GPIO5       SD_CS
//      GPIO18      SD_CLK
//      GPIO19      SD_MISO
//      GPIO23      SD_MOSI