EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
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
L Connector:Conn_01x05_Male J1
U 1 1 60290B8D
P 4900 3500
F 0 "J1" V 4850 3550 50  0000 R CNN
F 1 "v1 programmer" V 4750 3800 50  0000 R CNN
F 2 "earthvenusmars:programmer_pins_v1" H 4900 3500 50  0001 C CNN
F 3 "~" H 4900 3500 50  0001 C CNN
	1    4900 3500
	1    0    0    -1  
$EndComp
Connection ~ 5100 4350
$Comp
L power:GND #PWR0101
U 1 1 602A79C5
P 5100 4350
F 0 "#PWR0101" H 5100 4100 50  0001 C CNN
F 1 "GND" H 5105 4177 50  0000 C CNN
F 2 "" H 5100 4350 50  0001 C CNN
F 3 "" H 5100 4350 50  0001 C CNN
	1    5100 4350
	1    0    0    -1  
$EndComp
Connection ~ 5100 3950
$Comp
L power:+3.3V #PWR0102
U 1 1 602A7493
P 5100 3950
F 0 "#PWR0102" H 5100 3800 50  0001 C CNN
F 1 "+3.3V" H 5050 4100 50  0000 C CNN
F 2 "" H 5100 3950 50  0001 C CNN
F 3 "" H 5100 3950 50  0001 C CNN
	1    5100 3950
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x05_Male J2
U 1 1 60294536
P 4900 4150
F 0 "J2" V 4850 4200 50  0000 R CNN
F 1 "v2 programmer" V 4750 4400 50  0000 R CNN
F 2 "earthvenusmars:programmer_pins_v2" H 4900 4150 50  0001 C CNN
F 3 "~" H 4900 4150 50  0001 C CNN
	1    4900 4150
	1    0    0    -1  
$EndComp
Wire Wire Line
	5300 3300 5300 3950
Wire Wire Line
	5350 3400 5350 4050
Wire Wire Line
	5400 3500 5400 4150
Wire Wire Line
	5450 3600 5450 4250
Wire Wire Line
	5500 3700 5500 4350
Text Label 5100 4050 0    50   ~ 0
SWDIO
Text Label 5100 4150 0    50   ~ 0
SWCLK
Text Label 5100 4250 0    50   ~ 0
RESET
Wire Wire Line
	5100 3300 5300 3300
Wire Wire Line
	5100 3400 5350 3400
Wire Wire Line
	5100 3500 5400 3500
Wire Wire Line
	5100 3600 5450 3600
Wire Wire Line
	5100 3700 5500 3700
Wire Wire Line
	5100 3950 5300 3950
Wire Wire Line
	5100 4050 5350 4050
Wire Wire Line
	5100 4150 5400 4150
Wire Wire Line
	5100 4250 5450 4250
Wire Wire Line
	5100 4350 5500 4350
NoConn ~ 5950 3350
Wire Wire Line
	6150 3350 6150 3250
Wire Wire Line
	6150 3250 6050 3250
Wire Wire Line
	6050 3250 6050 3350
Wire Wire Line
	5850 3250 5850 3350
Connection ~ 6050 3250
$Comp
L power:GND #PWR0103
U 1 1 602B1F7A
P 5850 3250
F 0 "#PWR0103" H 5850 3000 50  0001 C CNN
F 1 "GND" V 5855 3122 50  0000 R CNN
F 2 "" H 5850 3250 50  0001 C CNN
F 3 "" H 5850 3250 50  0001 C CNN
	1    5850 3250
	-1   0    0    1   
$EndComp
Connection ~ 5850 3250
$Comp
L power:+3.3V #PWR0104
U 1 1 602B258D
P 6250 3350
F 0 "#PWR0104" H 6250 3200 50  0001 C CNN
F 1 "+3.3V" V 6265 3478 50  0000 L CNN
F 2 "" H 6250 3350 50  0001 C CNN
F 3 "" H 6250 3350 50  0001 C CNN
	1    6250 3350
	1    0    0    -1  
$EndComp
Connection ~ 5350 4050
Connection ~ 5400 4150
Connection ~ 5450 4250
NoConn ~ 5950 3850
NoConn ~ 6050 3850
Wire Wire Line
	5450 4250 5850 4250
Wire Wire Line
	5850 3850 5850 4250
Wire Wire Line
	6150 4150 6150 3850
Wire Wire Line
	5400 4150 6150 4150
Wire Wire Line
	5350 4050 6250 4050
Wire Wire Line
	6250 3850 6250 4050
$Comp
L Connector_Generic:Conn_02x05_Odd_Even SWD1
U 1 1 602AB4EE
P 6050 3650
F 0 "SWD1" H 6100 4067 50  0000 C CNN
F 1 "Conn_02x05_Top_Bottom" H 6000 3950 50  0000 C CNN
F 2 "Connector_PinHeader_1.27mm:PinHeader_2x05_P1.27mm_Vertical_SMD" H 6050 3650 50  0001 C CNN
F 3 "~" H 6050 3650 50  0001 C CNN
	1    6050 3650
	0    -1   -1   0   
$EndComp
Wire Wire Line
	5850 3250 6050 3250
$Comp
L Switch:SW_SPST SW1
U 1 1 602D0474
P 5800 4600
F 0 "SW1" H 5800 4835 50  0000 C CNN
F 1 "SW_SPST" H 5800 4744 50  0000 C CNN
F 2 "Button_Switch_SMD:SW_SPDT_PCM12" H 5800 4600 50  0001 C CNN
F 3 "~" H 5800 4600 50  0001 C CNN
	1    5800 4600
	1    0    0    -1  
$EndComp
Wire Wire Line
	5500 4350 5500 4600
Wire Wire Line
	5500 4600 5600 4600
Connection ~ 5500 4350
Wire Wire Line
	5850 4250 6000 4250
Wire Wire Line
	6000 4250 6000 4600
Connection ~ 5850 4250
$Comp
L Connector:Conn_01x02_Female JP1
U 1 1 6027DF78
P 5250 2700
F 0 "JP1" V 5300 2650 50  0000 C CNN
F 1 "Conn_01x02_Female" V 5400 2650 50  0000 C CNN
F 2 "Pad_SMD:Pad_SMD_01x02" H 5250 2700 50  0001 C CNN
F 3 "~" H 5250 2700 50  0001 C CNN
	1    5250 2700
	-1   0    0    1   
$EndComp
$Comp
L power:GND #PWR0105
U 1 1 60280C7F
P 5450 2600
F 0 "#PWR0105" H 5450 2350 50  0001 C CNN
F 1 "GND" V 5455 2472 50  0000 R CNN
F 2 "" H 5450 2600 50  0001 C CNN
F 3 "" H 5450 2600 50  0001 C CNN
	1    5450 2600
	0    -1   -1   0   
$EndComp
$Comp
L power:+3.3V #PWR0106
U 1 1 602811CF
P 5450 2700
F 0 "#PWR0106" H 5450 2550 50  0001 C CNN
F 1 "+3.3V" V 5465 2828 50  0000 L CNN
F 2 "" H 5450 2700 50  0001 C CNN
F 3 "" H 5450 2700 50  0001 C CNN
	1    5450 2700
	0    1    1    0   
$EndComp
$EndSCHEMATC
