EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 2
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L power:GND #PWR0101
U 1 1 5EB2B280
P 3150 1400
F 0 "#PWR0101" H 3150 1150 50  0001 C CNN
F 1 "GND" H 3155 1227 50  0000 C CNN
F 2 "" H 3150 1400 50  0001 C CNN
F 3 "" H 3150 1400 50  0001 C CNN
	1    3150 1400
	-1   0    0    1   
$EndComp
$Comp
L power:GND #PWR0102
U 1 1 5EB2B290
P 3550 5250
F 0 "#PWR0102" H 3550 5000 50  0001 C CNN
F 1 "GND" V 3555 5122 50  0000 R CNN
F 2 "" H 3550 5250 50  0001 C CNN
F 3 "" H 3550 5250 50  0001 C CNN
	1    3550 5250
	-1   0    0    1   
$EndComp
$Comp
L Regulator_Linear:MIC5504-3.3YM5 U2
U 1 1 5EB2B35B
P 1850 1300
F 0 "U2" H 1850 1667 50  0000 C CNN
F 1 "MIC5504-3.3YM5" H 1850 1576 50  0000 C CNN
F 2 "Package_TO_SOT_SMD:SOT-23-5" H 1850 900 50  0001 C CNN
F 3 "http://ww1.microchip.com/downloads/en/DeviceDoc/MIC550X.pdf" H 1600 1550 50  0001 C CNN
	1    1850 1300
	1    0    0    -1  
$EndComp
$Comp
L Device:Crystal_Small Y1
U 1 1 5EB55ACF
P 3450 1650
F 0 "Y1" V 3404 1738 50  0000 L CNN
F 1 "32768Hz" V 3495 1738 50  0000 L CNN
F 2 "Crystal:Crystal_SMD_3215-2Pin_3.2x1.5mm" H 3450 1650 50  0001 C CNN
F 3 "~" H 3450 1650 50  0001 C CNN
F 4 "ECS-.327-12.5-34B-TR" H 3450 1650 50  0001 C CNN "Designation"
	1    3450 1650
	0    1    1    0   
$EndComp
Connection ~ 3450 1550
Wire Wire Line
	3400 1750 3450 1750
Connection ~ 3450 1750
Wire Wire Line
	3400 1550 3450 1550
$Comp
L Device:C_Small C2
U 1 1 5EB59995
P 3300 1550
F 0 "C2" V 3071 1550 50  0000 C CNN
F 1 "15pF" V 3162 1550 50  0000 C CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 3300 1550 50  0001 C CNN
F 3 "~" H 3300 1550 50  0001 C CNN
	1    3300 1550
	0    1    1    0   
$EndComp
Wire Wire Line
	5350 4050 5400 4050
Wire Wire Line
	5400 4050 5400 4150
Wire Wire Line
	5350 4150 5400 4150
Connection ~ 5400 4150
Wire Wire Line
	5400 4150 5400 4250
Wire Wire Line
	5350 4250 5400 4250
Connection ~ 5400 4250
Wire Wire Line
	5400 4250 5400 4350
Wire Wire Line
	5350 4350 5400 4350
Connection ~ 5400 4350
Wire Wire Line
	5400 4350 5400 4400
$Comp
L power:GND #PWR0104
U 1 1 5EB634FD
P 5400 4400
F 0 "#PWR0104" H 5400 4150 50  0001 C CNN
F 1 "GND" H 5405 4227 50  0000 C CNN
F 2 "" H 5400 4400 50  0001 C CNN
F 3 "" H 5400 4400 50  0001 C CNN
	1    5400 4400
	1    0    0    -1  
$EndComp
$Comp
L Device:C_Small C3
U 1 1 5EB69322
P 5650 1050
F 0 "C3" H 5559 1004 50  0000 R CNN
F 1 "100nF" H 5559 1095 50  0000 R CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 5650 1050 50  0001 C CNN
F 3 "~" H 5650 1050 50  0001 C CNN
	1    5650 1050
	1    0    0    1   
$EndComp
$Comp
L Device:C_Small C4
U 1 1 5EB6981F
P 5800 1050
F 0 "C4" H 5892 1096 50  0000 L CNN
F 1 "10uF" H 5892 1005 50  0000 L CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 5800 1050 50  0001 C CNN
F 3 "~" H 5800 1050 50  0001 C CNN
	1    5800 1050
	1    0    0    -1  
$EndComp
Wire Wire Line
	5350 1150 5650 1150
Wire Wire Line
	5800 1150 5650 1150
Connection ~ 5650 1150
Wire Wire Line
	5650 950  5800 950 
Wire Wire Line
	5800 950  5950 950 
Connection ~ 5800 950 
Connection ~ 5800 1150
Wire Wire Line
	5350 1750 5400 1750
Wire Wire Line
	5500 1750 5500 1950
$Comp
L Device:C_Small C7
U 1 1 5EB83BDB
P 5500 2050
F 0 "C7" H 5408 2096 50  0000 R CNN
F 1 "1uF" H 5408 2005 50  0000 R CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 5500 2050 50  0001 C CNN
F 3 "~" H 5500 2050 50  0001 C CNN
	1    5500 2050
	-1   0    0    -1  
$EndComp
Wire Wire Line
	5500 2150 6000 2150
$Comp
L power:GND #PWR0109
U 1 1 5EB8A527
P 6000 2150
F 0 "#PWR0109" H 6000 1900 50  0001 C CNN
F 1 "GND" V 6005 2022 50  0000 R CNN
F 2 "" H 6000 2150 50  0001 C CNN
F 3 "" H 6000 2150 50  0001 C CNN
	1    6000 2150
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR0110
U 1 1 5EB8AC82
P 6500 1700
F 0 "#PWR0110" H 6500 1450 50  0001 C CNN
F 1 "GND" H 6505 1527 50  0000 C CNN
F 2 "" H 6500 1700 50  0001 C CNN
F 3 "" H 6500 1700 50  0001 C CNN
	1    6500 1700
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR0111
U 1 1 5EB8BB7C
P 6500 1500
F 0 "#PWR0111" H 6500 1350 50  0001 C CNN
F 1 "+3.3V" H 6515 1673 50  0000 C CNN
F 2 "" H 6500 1500 50  0001 C CNN
F 3 "" H 6500 1500 50  0001 C CNN
	1    6500 1500
	1    0    0    -1  
$EndComp
$Comp
L Device:C_Small C8
U 1 1 5EB8BF71
P 6500 1600
F 0 "C8" H 6408 1646 50  0000 R CNN
F 1 "10uF" H 6408 1555 50  0000 R CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 6500 1600 50  0001 C CNN
F 3 "~" H 6500 1600 50  0001 C CNN
	1    6500 1600
	-1   0    0    -1  
$EndComp
Wire Wire Line
	5800 1150 6150 1150
$Comp
L power:GND #PWR0112
U 1 1 5EB6D720
P 5950 950
F 0 "#PWR0112" H 5950 700 50  0001 C CNN
F 1 "GND" H 5955 777 50  0000 C CNN
F 2 "" H 5950 950 50  0001 C CNN
F 3 "" H 5950 950 50  0001 C CNN
	1    5950 950 
	-1   0    0    1   
$EndComp
$Comp
L Device:C_Small C1
U 1 1 5EB584EB
P 3300 1750
F 0 "C1" V 3100 1750 50  0000 C CNN
F 1 "15pF" V 3183 1750 50  0000 C CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 3300 1750 50  0001 C CNN
F 3 "~" H 3300 1750 50  0001 C CNN
	1    3300 1750
	0    1    -1   0   
$EndComp
$Comp
L power:+3.3V #PWR0113
U 1 1 5EBA1A5C
P 6150 950
F 0 "#PWR0113" H 6150 800 50  0001 C CNN
F 1 "+3.3V" H 6165 1123 50  0000 C CNN
F 2 "" H 6150 950 50  0001 C CNN
F 3 "" H 6150 950 50  0001 C CNN
	1    6150 950 
	1    0    0    -1  
$EndComp
Wire Wire Line
	3450 1550 3950 1550
Wire Wire Line
	3150 1550 3200 1550
Wire Wire Line
	3200 1750 3150 1750
Wire Wire Line
	3150 1750 3150 1550
Wire Wire Line
	3150 1400 3150 1550
Connection ~ 3150 1550
$Comp
L Device:Ferrite_Bead_Small FB1
U 1 1 5EBB1927
P 6150 1050
F 0 "FB1" H 6250 1096 50  0000 L CNN
F 1 "220Ω @ 100MHz" H 6250 1005 50  0000 L CNN
F 2 "Inductor_SMD:L_0603_1608Metric" V 6080 1050 50  0001 C CNN
F 3 "~" H 6150 1050 50  0001 C CNN
F 4 "BLM18AG221SN1D" H 6150 1050 50  0001 C CNN "Designation"
	1    6150 1050
	1    0    0    -1  
$EndComp
$Comp
L Device:C_Small C9
U 1 1 5EBB485C
P 1150 1400
F 0 "C9" H 1058 1446 50  0000 R CNN
F 1 "1uF" H 1058 1355 50  0000 R CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 1150 1400 50  0001 C CNN
F 3 "~" H 1150 1400 50  0001 C CNN
	1    1150 1400
	-1   0    0    -1  
$EndComp
Wire Wire Line
	1850 1600 1850 1700
$Comp
L power:GND #PWR0114
U 1 1 5EBB72AE
P 1850 1750
F 0 "#PWR0114" H 1850 1500 50  0001 C CNN
F 1 "GND" H 1855 1577 50  0000 C CNN
F 2 "" H 1850 1750 50  0001 C CNN
F 3 "" H 1850 1750 50  0001 C CNN
	1    1850 1750
	1    0    0    -1  
$EndComp
Wire Wire Line
	1850 1700 1150 1700
Wire Wire Line
	1150 1700 1150 1500
Connection ~ 1850 1700
Wire Wire Line
	1850 1700 1850 1750
Wire Wire Line
	1150 1300 1150 1200
Wire Wire Line
	1150 1200 1450 1200
$Comp
L Device:C_Small C10
U 1 1 5EBC1020
P 2400 1400
F 0 "C10" H 2308 1446 50  0000 R CNN
F 1 "1uF" H 2308 1355 50  0000 R CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 2400 1400 50  0001 C CNN
F 3 "~" H 2400 1400 50  0001 C CNN
	1    2400 1400
	-1   0    0    -1  
$EndComp
Wire Wire Line
	2250 1200 2400 1200
Wire Wire Line
	2400 1200 2400 1300
Wire Wire Line
	1850 1700 2400 1700
Wire Wire Line
	2400 1500 2400 1700
Wire Wire Line
	2400 1200 2600 1200
Connection ~ 2400 1200
Wire Wire Line
	1150 1200 1000 1200
Connection ~ 1150 1200
$Comp
L power:+3.3V #PWR0116
U 1 1 5EBCC949
P 2600 1200
F 0 "#PWR0116" H 2600 1050 50  0001 C CNN
F 1 "+3.3V" H 2615 1373 50  0000 C CNN
F 2 "" H 2600 1200 50  0001 C CNN
F 3 "" H 2600 1200 50  0001 C CNN
	1    2600 1200
	1    0    0    -1  
$EndComp
Wire Wire Line
	1450 1400 1450 1200
Connection ~ 1450 1200
Text Notes 1450 800  0    50   ~ 0
USB Power Regulator
Text GLabel 3950 1350 0    50   Input ~ 0
RESET
Wire Wire Line
	3450 1750 3950 1750
Wire Wire Line
	3950 1550 3950 1650
$Comp
L power:PWR_FLAG #FLG0101
U 1 1 5EC795FC
P 5400 1750
F 0 "#FLG0101" H 5400 1825 50  0001 C CNN
F 1 "PWR_FLAG" V 5400 2000 50  0000 C CNN
F 2 "" H 5400 1750 50  0001 C CNN
F 3 "~" H 5400 1750 50  0001 C CNN
	1    5400 1750
	-1   0    0    1   
$EndComp
$Comp
L power:PWR_FLAG #FLG0102
U 1 1 5EC80926
P 5350 1150
F 0 "#FLG0102" H 5350 1225 50  0001 C CNN
F 1 "PWR_FLAG" H 5300 1300 50  0000 L CNN
F 2 "" H 5350 1150 50  0001 C CNN
F 3 "~" H 5350 1150 50  0001 C CNN
	1    5350 1150
	0    -1   -1   0   
$EndComp
$Comp
L Device:C_Small C6
U 1 1 5EB80237
P 5700 1750
F 0 "C6" H 5608 1796 50  0000 R CNN
F 1 "100nF" H 5608 1705 50  0000 R CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 5700 1750 50  0001 C CNN
F 3 "~" H 5700 1750 50  0001 C CNN
	1    5700 1750
	-1   0    0    -1  
$EndComp
Wire Wire Line
	5700 1650 6000 1650
$Comp
L power:+3.3V #PWR0107
U 1 1 5EB81874
P 6000 1650
F 0 "#PWR0107" H 6000 1500 50  0001 C CNN
F 1 "+3.3V" V 6015 1778 50  0000 L CNN
F 2 "" H 6000 1650 50  0001 C CNN
F 3 "" H 6000 1650 50  0001 C CNN
	1    6000 1650
	0    1    1    0   
$EndComp
Wire Wire Line
	5700 1850 6000 1850
$Comp
L power:GND #PWR0108
U 1 1 5EB8329F
P 6000 1850
F 0 "#PWR0108" H 6000 1600 50  0001 C CNN
F 1 "GND" V 6005 1722 50  0000 R CNN
F 2 "" H 6000 1850 50  0001 C CNN
F 3 "" H 6000 1850 50  0001 C CNN
	1    6000 1850
	0    -1   -1   0   
$EndComp
Connection ~ 5700 1650
$Comp
L power:+3.3V #PWR0106
U 1 1 5EB7D0AC
P 5950 1550
F 0 "#PWR0106" H 5950 1400 50  0001 C CNN
F 1 "+3.3V" V 5965 1678 50  0000 L CNN
F 2 "" H 5950 1550 50  0001 C CNN
F 3 "" H 5950 1550 50  0001 C CNN
	1    5950 1550
	0    1    1    0   
$EndComp
$Comp
L power:GND #PWR0105
U 1 1 5EB7BFC2
P 5950 1350
F 0 "#PWR0105" H 5950 1100 50  0001 C CNN
F 1 "GND" V 5955 1222 50  0000 R CNN
F 2 "" H 5950 1350 50  0001 C CNN
F 3 "" H 5950 1350 50  0001 C CNN
	1    5950 1350
	0    -1   -1   0   
$EndComp
$Comp
L Device:C_Small C5
U 1 1 5EB7A3B0
P 5700 1450
F 0 "C5" H 5608 1496 50  0000 R CNN
F 1 "100nF" H 5608 1405 50  0000 R CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 5700 1450 50  0001 C CNN
F 3 "~" H 5700 1450 50  0001 C CNN
	1    5700 1450
	-1   0    0    -1  
$EndComp
Wire Wire Line
	5350 1650 5700 1650
Wire Wire Line
	5350 1350 5350 1150
Connection ~ 5350 1150
Wire Wire Line
	5350 1450 5350 1550
Text Label 5500 1150 3    50   ~ 0
VDDANA
Wire Wire Line
	5700 1350 5950 1350
Wire Wire Line
	5700 1550 5950 1550
Wire Wire Line
	5350 1550 5700 1550
Connection ~ 5700 1550
Text Label 5350 1750 3    50   ~ 0
VDDCORE
Connection ~ 5400 1750
Wire Wire Line
	5400 1750 5500 1750
Text Label 3650 4150 0    50   ~ 0
USB_D+
Text Label 3950 4050 2    50   ~ 0
USB_D-
NoConn ~ 3950 2950
NoConn ~ 3950 2850
NoConn ~ 3950 2450
NoConn ~ 3950 1950
NoConn ~ 3950 1850
NoConn ~ 5350 2350
NoConn ~ 5350 2450
NoConn ~ 5350 2650
NoConn ~ 5350 2750
NoConn ~ 5350 2850
NoConn ~ 5350 2950
NoConn ~ 3950 3450
NoConn ~ 3950 3250
NoConn ~ 3950 3150
NoConn ~ 3950 4350
Wire Wire Line
	3950 3350 3700 3350
NoConn ~ 3700 3350
Text Label 3950 3350 2    50   ~ 0
PA17_MISO
$Sheet
S 2800 3750 400  800 
U 5F81AC32
F0 "APA102c LED Array" 50
F1 "leds.sch" 50
F2 "LED_SCK" I T 3050 3750 50 
F3 "LED_DATA" I T 3150 3750 50 
F4 "+5V" I T 2950 3750 50 
F5 "GND" I T 2850 3750 50 
$EndSheet
$Comp
L power:GND #PWR0127
U 1 1 5FBE7E85
P 2850 3650
F 0 "#PWR0127" H 2850 3400 50  0001 C CNN
F 1 "GND" H 2900 3500 50  0000 C CNN
F 2 "" H 2850 3650 50  0001 C CNN
F 3 "" H 2850 3650 50  0001 C CNN
	1    2850 3650
	-1   0    0    1   
$EndComp
$Comp
L Switch:SW_SPST SW1
U 1 1 5FC0D3F5
P 6350 3000
F 0 "SW1" H 6350 3235 50  0000 C CNN
F 1 "SW_SPST" H 6350 3144 50  0000 C CNN
F 2 "Button_Switch_SMD:KMR731NG_LFS" H 6350 3000 50  0001 C CNN
F 3 "~" H 6350 3000 50  0001 C CNN
F 4 "KMR731NGLFS" H 6350 3000 50  0001 C CNN "Designation"
	1    6350 3000
	1    0    0    -1  
$EndComp
Connection ~ 5350 1550
Text Label 3800 3850 0    50   ~ 0
SDA
Text Label 3800 3950 0    50   ~ 0
SCL
$Comp
L Mic_I2S_SPH0645LM4H-B:SPH0645LM4H-B MK1
U 1 1 5F90704B
P 1700 3200
F 0 "MK1" H 1700 2750 50  0000 C CNN
F 1 "SPH0645LM4H-B" H 1700 2850 50  0000 C CNN
F 2 "Mic-SPH0645LM4H-B:MIC_SPH0645LM4H-B" H 1700 3200 50  0001 L BNN
F 3 "" H 1700 3200 50  0001 L BNN
F 4 "Mic Mems Digital I2s Omni -26db" H 1700 3200 50  0001 L BNN "Field4"
F 5 "SPH0645LM4H-B" H 1700 3200 50  0001 L BNN "Field5"
F 6 "SIP-7 Bourns" H 1700 3200 50  0001 L BNN "Field6"
F 7 "None" H 1700 3200 50  0001 L BNN "Field7"
F 8 "Unavailable" H 1700 3200 50  0001 L BNN "Field8"
	1    1700 3200
	0    -1   -1   0   
$EndComp
$Comp
L Device:R R1
U 1 1 5F907051
P 1850 2350
F 0 "R1" V 1750 2350 50  0000 C CNN
F 1 "68" V 1950 2350 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric" V 1780 2350 50  0001 C CNN
F 3 "~" H 1850 2350 50  0001 C CNN
	1    1850 2350
	0    1    1    0   
$EndComp
Wire Wire Line
	1700 3900 1700 4000
Text Label 3400 2750 0    50   ~ 0
MIC_LRCLK
$Comp
L Device:C C13
U 1 1 5F907060
P 1150 2850
F 0 "C13" H 1150 2950 50  0000 L CNN
F 1 "1uF" H 1100 2750 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 1188 2700 50  0001 C CNN
F 3 "~" H 1150 2850 50  0001 C CNN
	1    1150 2850
	0    -1   -1   0   
$EndComp
$Comp
L Device:C C12
U 1 1 5F907066
P 1150 2500
F 0 "C12" H 1000 2600 50  0000 L CNN
F 1 "220pF" H 1000 2400 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 1188 2350 50  0001 C CNN
F 3 "~" H 1150 2500 50  0001 C CNN
	1    1150 2500
	0    -1   -1   0   
$EndComp
Wire Wire Line
	1300 2500 1300 2850
$Comp
L power:GND #PWR0129
U 1 1 5F90706F
P 1900 2500
F 0 "#PWR0129" H 1900 2250 50  0001 C CNN
F 1 "GND" H 1905 2327 50  0000 C CNN
F 2 "" H 1900 2500 50  0001 C CNN
F 3 "" H 1900 2500 50  0001 C CNN
	1    1900 2500
	0    -1   -1   0   
$EndComp
Wire Wire Line
	1000 2850 1000 2500
$Comp
L power:GND #PWR0130
U 1 1 5F907076
P 1000 2850
F 0 "#PWR0130" H 1000 2600 50  0001 C CNN
F 1 "GND" H 1005 2677 50  0000 C CNN
F 2 "" H 1000 2850 50  0001 C CNN
F 3 "" H 1000 2850 50  0001 C CNN
	1    1000 2850
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0131
U 1 1 5F90707C
P 1700 4300
F 0 "#PWR0131" H 1700 4050 50  0001 C CNN
F 1 "GND" V 1705 4172 50  0000 R CNN
F 2 "" H 1700 4300 50  0001 C CNN
F 3 "" H 1700 4300 50  0001 C CNN
	1    1700 4300
	1    0    0    -1  
$EndComp
Connection ~ 1000 2850
Connection ~ 1300 2500
Wire Wire Line
	3150 3650 3150 3750
Wire Wire Line
	3150 3650 3950 3650
Wire Wire Line
	3050 3750 3050 3550
Wire Wire Line
	2950 3750 2950 3650
Wire Wire Line
	1300 2500 1500 2500
Wire Wire Line
	1700 2350 1700 2500
$Comp
L Device:R R2
U 1 1 5F907057
P 1700 4150
F 0 "R2" V 1600 4150 50  0000 C CNN
F 1 "10K" V 1800 4150 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 1630 4150 50  0001 C CNN
F 3 "~" H 1700 4150 50  0001 C CNN
	1    1700 4150
	1    0    0    -1  
$EndComp
Wire Wire Line
	2200 3900 1800 3900
Wire Wire Line
	2200 2650 3950 2650
Wire Wire Line
	2250 3950 1600 3950
Wire Wire Line
	1600 3950 1600 3900
Wire Wire Line
	2250 2750 3950 2750
Text Label 3400 2650 0    50   ~ 0
MIC_SCK_BCLK
Text Label 3400 2350 0    50   ~ 0
MIC_DATA
Wire Wire Line
	2000 2350 3950 2350
Wire Wire Line
	2200 2650 2200 3900
Wire Wire Line
	2250 2750 2250 3950
$Comp
L power:+3.3V #PWR0132
U 1 1 5F97EEE1
P 1300 2850
F 0 "#PWR0132" H 1300 2700 50  0001 C CNN
F 1 "+3.3V" H 1315 3023 50  0000 C CNN
F 2 "" H 1300 2850 50  0001 C CNN
F 3 "" H 1300 2850 50  0001 C CNN
	1    1300 2850
	-1   0    0    1   
$EndComp
Connection ~ 1300 2850
$Comp
L Switch:SW_SPST SW2
U 1 1 5F9106F9
P 6350 3350
F 0 "SW2" H 6350 3585 50  0000 C CNN
F 1 "SW_SPST" H 6350 3494 50  0000 C CNN
F 2 "Button_Switch_SMD:KMR731NG_LFS" H 6350 3350 50  0001 C CNN
F 3 "~" H 6350 3350 50  0001 C CNN
F 4 "KMR731NGLFS" H 6350 3350 50  0001 C CNN "Designation"
	1    6350 3350
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0133
U 1 1 5F9110A0
P 6550 3000
F 0 "#PWR0133" H 6550 2750 50  0001 C CNN
F 1 "GND" V 6555 2872 50  0000 R CNN
F 2 "" H 6550 3000 50  0001 C CNN
F 3 "" H 6550 3000 50  0001 C CNN
	1    6550 3000
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR0134
U 1 1 5F911146
P 6550 3350
F 0 "#PWR0134" H 6550 3100 50  0001 C CNN
F 1 "GND" V 6555 3222 50  0000 R CNN
F 2 "" H 6550 3350 50  0001 C CNN
F 3 "" H 6550 3350 50  0001 C CNN
	1    6550 3350
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR0135
U 1 1 5F9113D1
P 6300 4400
F 0 "#PWR0135" H 6300 4150 50  0001 C CNN
F 1 "GND" V 6305 4272 50  0000 R CNN
F 2 "" H 6300 4400 50  0001 C CNN
F 3 "" H 6300 4400 50  0001 C CNN
	1    6300 4400
	0    -1   -1   0   
$EndComp
Text GLabel 3950 3050 0    50   Input ~ 0
BUTTON_1
Text GLabel 3950 4250 0    50   Input ~ 0
BUTTON_2
Text GLabel 6150 3350 0    50   Input ~ 0
BUTTON_2
Text GLabel 6150 3000 0    50   Input ~ 0
BUTTON_1
$Comp
L Device:R_POT RV1
U 1 1 5FA0CDD3
P 6300 4550
F 0 "RV1" H 6230 4504 50  0000 R CNN
F 1 "R_POT" H 6230 4595 50  0000 R CNN
F 2 "Pot_dial_3352T-1-103LF:Pot_dial_3352T-1-103LF" H 6300 4550 50  0001 C CNN
F 3 "~" H 6300 4550 50  0001 C CNN
	1    6300 4550
	-1   0    0    1   
$EndComp
Text GLabel 3950 2550 0    50   Input ~ 0
DIAL_1
Text GLabel 6150 4550 0    50   Input ~ 0
DIAL_1
$Comp
L power:+3.3V #PWR0136
U 1 1 5FBE3505
P 6300 4700
F 0 "#PWR0136" H 6300 4550 50  0001 C CNN
F 1 "+3.3V" V 6315 4828 50  0000 L CNN
F 2 "" H 6300 4700 50  0001 C CNN
F 3 "" H 6300 4700 50  0001 C CNN
	1    6300 4700
	0    1    1    0   
$EndComp
$Comp
L Connector:Conn_01x05_Female J1
U 1 1 5FBFDA7E
P 4600 5000
F 0 "J1" V 4500 4800 50  0000 L CNN
F 1 "Conn_01x05_Female" V 4650 4500 50  0000 L CNN
F 2 "earthvenusmars:programmer_pads_v2" H 4600 5000 50  0001 C CNN
F 3 "~" H 4600 5000 50  0001 C CNN
	1    4600 5000
	1    0    0    -1  
$EndComp
$Comp
L ATSAMD21G18A-AU:ATSAMD21G18A-AU U1
U 1 1 5EB21719
P 4650 2950
F 0 "U1" H 4650 4817 50  0000 C CNN
F 1 "ATSAMD21G18A-AU" H 4650 4726 50  0000 C CNN
F 2 "ATSAMD21G18A-AU:ATSAMD21G18A-AU" H 4650 2950 50  0001 L BNN
F 3 "ATMEL" H 4650 2950 50  0001 L BNN
	1    4650 2950
	1    0    0    -1  
$EndComp
Wire Wire Line
	3950 4450 3900 4450
Wire Wire Line
	4200 5100 4400 5100
Text GLabel 4200 5100 0    50   Input ~ 0
RESET
Wire Wire Line
	3900 5000 4400 5000
Wire Wire Line
	3900 4450 3900 5000
Wire Wire Line
	3950 4550 3950 4900
Wire Wire Line
	3950 4900 4400 4900
Text Label 4000 4900 0    50   ~ 0
SWDIO
Text Label 4000 5000 0    50   ~ 0
SWCLK
$Comp
L power:GND #PWR0138
U 1 1 5FC4F9CF
P 4400 5200
F 0 "#PWR0138" H 4400 4950 50  0001 C CNN
F 1 "GND" V 4405 5072 50  0000 R CNN
F 2 "" H 4400 5200 50  0001 C CNN
F 3 "" H 4400 5200 50  0001 C CNN
	1    4400 5200
	0    1    1    0   
$EndComp
$Comp
L power:+3.3V #PWR0139
U 1 1 5FC5014A
P 4400 4800
F 0 "#PWR0139" H 4400 4650 50  0001 C CNN
F 1 "+3.3V" V 4415 4928 50  0000 L CNN
F 2 "" H 4400 4800 50  0001 C CNN
F 3 "" H 4400 4800 50  0001 C CNN
	1    4400 4800
	0    -1   -1   0   
$EndComp
Wire Wire Line
	3500 4050 3950 4050
Wire Wire Line
	3600 4150 3950 4150
Wire Wire Line
	3050 3550 3950 3550
Text GLabel 3950 2250 0    50   Input ~ 0
TOUCH_PIN_1
NoConn ~ 3950 3750
Text GLabel 3950 2050 0    50   Input ~ 0
TOUCH_PIN_2
Text GLabel 6300 5100 0    50   Input ~ 0
TOUCH_PIN_2
Text GLabel 6300 5300 0    50   Input ~ 0
TOUCH_PIN_3
$Comp
L Connector:Conn_01x01_Female J_T2
U 1 1 60BC2291
P 6500 5100
F 0 "J_T2" H 6528 5126 50  0000 L CNN
F 1 "Conn_01x01_Female" H 6528 5035 50  0000 L CNN
F 2 "earthvenusmars:touch_venus" H 6500 5100 50  0001 C CNN
F 3 "~" H 6500 5100 50  0001 C CNN
	1    6500 5100
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x01_Female J_T3
U 1 1 60BC3BD3
P 6500 5300
F 0 "J_T3" H 6528 5326 50  0000 L CNN
F 1 "Conn_01x01_Female" H 6528 5235 50  0000 L CNN
F 2 "earthvenusmars:touch_mars" H 6500 5300 50  0001 C CNN
F 3 "~" H 6500 5300 50  0001 C CNN
	1    6500 5300
	1    0    0    -1  
$EndComp
Text GLabel 6300 4900 0    50   Input ~ 0
TOUCH_PIN_1
$Comp
L Connector:Conn_01x01_Female J_T1
U 1 1 60BC9517
P 6500 4900
F 0 "J_T1" H 6528 4926 50  0000 L CNN
F 1 "Conn_01x01_Female" H 6528 4835 50  0000 L CNN
F 2 "earthvenusmars:touch_earth" H 6500 4900 50  0001 C CNN
F 3 "~" H 6500 4900 50  0001 C CNN
	1    6500 4900
	1    0    0    -1  
$EndComp
Text GLabel 5350 2250 2    50   Input ~ 0
TOUCH_PIN_3
$Comp
L Connector:Conn_01x02_Female J_LED1
U 1 1 60BA94AC
P 3050 3150
F 0 "J_LED1" V 3100 3200 50  0000 R CNN
F 1 "Conn_01x02_Female" V 3000 3450 50  0000 R CNN
F 2 "earthvenusmars:SMD_Pad_01x02_1.27mm" H 3050 3150 50  0001 C CNN
F 3 "~" H 3050 3150 50  0001 C CNN
	1    3050 3150
	0    -1   -1   0   
$EndComp
Wire Wire Line
	3150 3350 3150 3650
Connection ~ 3150 3650
Wire Wire Line
	3050 3350 3050 3550
Connection ~ 3050 3550
$Comp
L Connector:Conn_01x02_Female J_I2C1
U 1 1 60BB2E0D
P 3250 2900
F 0 "J_I2C1" V 3300 2900 50  0000 R CNN
F 1 "Conn_01x02_Female" V 3200 3200 50  0000 R CNN
F 2 "earthvenusmars:SMD_Pad_01x02_1.27mm" H 3250 2900 50  0001 C CNN
F 3 "~" H 3250 2900 50  0001 C CNN
	1    3250 2900
	0    -1   -1   0   
$EndComp
Wire Wire Line
	3350 3850 3350 3100
Wire Wire Line
	3350 3850 3950 3850
Wire Wire Line
	3250 3100 3250 3950
Wire Wire Line
	3250 3950 3950 3950
NoConn ~ 3250 4950
NoConn ~ 3450 5350
$Comp
L Connector:USB_B_Micro J_USB_PORT1
U 1 1 60BC4B2E
P 3050 5250
F 0 "J_USB_PORT1" V 2700 5050 50  0000 L CNN
F 1 "USB_B_Micro" V 2800 5000 50  0000 L CNN
F 2 "Connector_USB:USB_Micro-B_GCT_USB3076-30-A" H 3200 5200 50  0001 C CNN
F 3 "~" H 3200 5200 50  0001 C CNN
	1    3050 5250
	0    -1   -1   0   
$EndComp
$Comp
L power:VBUS #PWR0103
U 1 1 5EB2B320
P 2850 4900
F 0 "#PWR0103" H 2850 4750 50  0001 C CNN
F 1 "VBUS" V 2865 5027 50  0000 L CNN
F 2 "" H 2850 4900 50  0001 C CNN
F 3 "" H 2850 4900 50  0001 C CNN
	1    2850 4900
	1    0    0    -1  
$EndComp
Wire Wire Line
	3150 4850 3150 4950
Wire Wire Line
	3050 4950 3050 4800
Wire Wire Line
	2850 4950 2850 4900
$Comp
L Device:Thermistor_NTC TH1
U 1 1 60C6FD5E
P 3000 2000
F 0 "TH1" H 3050 2200 50  0000 R CNN
F 1 "Thermistor_NTC" H 3250 2250 50  0000 R CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 3000 2050 50  0001 C CNN
F 3 "~" H 3000 2050 50  0001 C CNN
F 4 "ERTJ1VG103GA" H 3000 2000 50  0001 C CNN "Designation"
	1    3000 2000
	-1   0    0    1   
$EndComp
$Comp
L Device:R R3
U 1 1 60C7ADC4
P 2750 2000
F 0 "R3" H 2680 1954 50  0000 R CNN
F 1 "10K" H 2680 2045 50  0000 R CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 2680 2000 50  0001 C CNN
F 3 "~" H 2750 2000 50  0001 C CNN
	1    2750 2000
	-1   0    0    1   
$EndComp
Text GLabel 5350 2550 2    50   Input ~ 0
THERMISTOR_PWR
Text GLabel 2750 1850 1    50   Input ~ 0
THERMISTOR_PWR
Wire Wire Line
	3000 2150 3950 2150
$Comp
L power:GND #PWR0137
U 1 1 60C9B9A5
P 3000 1850
F 0 "#PWR0137" H 3000 1600 50  0001 C CNN
F 1 "GND" H 3005 1677 50  0000 C CNN
F 2 "" H 3000 1850 50  0001 C CNN
F 3 "" H 3000 1850 50  0001 C CNN
	1    3000 1850
	-1   0    0    1   
$EndComp
Connection ~ 3000 2150
Wire Wire Line
	2750 2150 3000 2150
Wire Wire Line
	3450 5250 3550 5250
Wire Wire Line
	3600 4150 3600 4850
Wire Wire Line
	3050 4800 3500 4800
Wire Wire Line
	3600 4850 3150 4850
Wire Wire Line
	3500 4050 3500 4800
Wire Wire Line
	2850 3650 2850 3750
$Comp
L power:+5V #PWR0140
U 1 1 60F182E5
P 2950 3650
F 0 "#PWR0140" H 2950 3500 50  0001 C CNN
F 1 "+5V" H 2965 3823 50  0000 C CNN
F 2 "" H 2950 3650 50  0001 C CNN
F 3 "" H 2950 3650 50  0001 C CNN
	1    2950 3650
	1    0    0    -1  
$EndComp
$Comp
L power:+5V #PWR0141
U 1 1 60F1EBB4
P 1000 1200
F 0 "#PWR0141" H 1000 1050 50  0001 C CNN
F 1 "+5V" H 1015 1373 50  0000 C CNN
F 2 "" H 1000 1200 50  0001 C CNN
F 3 "" H 1000 1200 50  0001 C CNN
	1    1000 1200
	1    0    0    -1  
$EndComp
NoConn ~ 6550 4050
$Comp
L power:VBUS #PWR0124
U 1 1 60ED4AF5
P 6150 3950
F 0 "#PWR0124" H 6150 3800 50  0001 C CNN
F 1 "VBUS" V 6165 4077 50  0000 L CNN
F 2 "" H 6150 3950 50  0001 C CNN
F 3 "" H 6150 3950 50  0001 C CNN
	1    6150 3950
	0    -1   -1   0   
$EndComp
$Comp
L power:+5V #PWR0115
U 1 1 60F0ACB9
P 6650 3850
F 0 "#PWR0115" H 6650 3700 50  0001 C CNN
F 1 "+5V" H 6665 4023 50  0000 C CNN
F 2 "" H 6650 3850 50  0001 C CNN
F 3 "" H 6650 3850 50  0001 C CNN
	1    6650 3850
	1    0    0    -1  
$EndComp
$Comp
L Device:C_Small C11
U 1 1 5FBCCF61
P 6750 3850
F 0 "C11" H 6658 3896 50  0000 R CNN
F 1 "10uF" H 6658 3805 50  0000 R CNN
F 2 "Capacitor_SMD:C_0402_1005Metric" H 6750 3850 50  0001 C CNN
F 3 "~" H 6750 3850 50  0001 C CNN
	1    6750 3850
	0    -1   -1   0   
$EndComp
$Comp
L Switch:SW_SPDT SW3
U 1 1 60EBA3E4
P 6350 3950
F 0 "SW3" H 6350 4235 50  0000 C CNN
F 1 "SW_SPDT" H 6350 4144 50  0000 C CNN
F 2 "earthvenusmars:COPAL-MFS101D-6-Z" H 6350 3950 50  0001 C CNN
F 3 "~" H 6350 3950 50  0001 C CNN
	1    6350 3950
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0142
U 1 1 60F4F7F8
P 6850 3850
F 0 "#PWR0142" H 6850 3600 50  0001 C CNN
F 1 "GND" H 6855 3677 50  0000 C CNN
F 2 "" H 6850 3850 50  0001 C CNN
F 3 "" H 6850 3850 50  0001 C CNN
	1    6850 3850
	0    -1   -1   0   
$EndComp
Wire Wire Line
	6550 3850 6650 3850
Connection ~ 6650 3850
$EndSCHEMATC
