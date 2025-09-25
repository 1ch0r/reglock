# RegLock

RegLock is a collaboration research project exploring the reverse engineering of traditional RFID lock systems and exploring
new avenues in programming these systems. Areas of exploration include over the air dynamic programming, rolling code, encryption
and time synchronization.

# Hardware

RF Modules: LoRa 896 Transceiver
Microcontroller: Arduino Uno R3
Computer: Raspberry Pi 4
Modules: Stepper Motor, NFC/RFID Module, Breadboard

# Software

Languages Used: C++, Python
IDE: Arduino IDE, PyCharm, Nano, Vim

# Philosophy

This project involved a 915MHz one way commmunication protocol using AT+ OTA message parsing via the LoRa.
The goal was to create a home station for controlling known registers on the lock. The home station is able
to remotely program the lock via these 915MHz messages. The application here is RFID lock systems where user
access can be remotely revoked with internet redundancy for secure access systems.

# Results

Results culminated in a promising and fully functional prototype in both hardware and software.
RF modules were able to communicate across residential buildings through dozens of walls and 
any floor up to 4 stories high. The current codebase is legacy code from the beginning of the project.
It is not guaranteed to be fully functional and is merely a somber relic of the early stages of RegLock.
