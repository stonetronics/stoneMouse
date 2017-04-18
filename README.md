Stonetronics StoneMouse

A vusb-based mouse using a joystick and some buttons

This repository contains
- EAGLE - schematic, board and library
- Source code and atmel studio project

The StoneMouse uses an ATmega8-Controller running at 12 MHz for vusb.
It collects data from an potentiometer joystick and calculates deltax and deltay of the mousecursor. There was the issue that when releasing the Joystick, the adc values of the potis didnt snap back to preciese center values. To fix this an iir filter was implemented in an interval around the center position to determine mean center position and stop the cursor from drifting

These buttons have been implemented:
- left mouse button
- right mouse button
- scroll up
- scroll down
- change precision

The pcb was designed to fit mostly through hole components mounted in an smd-like way, so that there are no spiky through hole junctions on the backside of the pcb.
This was done primaryly to prevent the user from being stung by a through hole junction and because i had only through hole parts lying around
USB-connection is done by an Mini-USB connector

Build summary:
The StoneMouse pcb was etched with natriumpersulfate and the components were soldered. The controller was flashed before it was soldered on the pcb.
For the Joystick, there was a prototyping pcb soldered 90° to the top of the base pcb to hold the components. Several buttons were soldered onto a prototyping pcb which was soldered to the base pcb and the joystick pcb. All buttons are debumped by an 100nF smd capacitor. The mechanical structure was strengthened by pieces of raw pcb soldered to the other pcbs, forming a housing for the electronics. 
