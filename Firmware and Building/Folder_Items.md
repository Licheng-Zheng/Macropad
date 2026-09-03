### Purpose of the folder
This is the folder with all of the code. Everything that is needed to compile the uf2 file (which is what gets brought into the board) is in here. Thereshould be 4 code files in total (see below for what they are). 

After the Raspberry Pi Vscode extension is activated, enter this folder, and click the compile button. THis should create a build file. 
- Within the build file, you will find a uf2 file, this is the code for the macropad. You can edit its function in main.cpp. 
- Unplug the raspberry pi pico, then replug it in while holding the `bootsel` button, this will make it ready to accept code that you send to it. This should make the raspberry pi appear as a drive in your file explorer. 
- Drag the uf2 file from your build folder into the raspberry pi in your file explorer, and it should disapear as a dirve. The code is now uploaded! Hopefully you've completed the soldering, and the macropad is done!

#### CMakeLists.txt
- Defines project details, enables the languages (C, C++, Assembly) used, and their standards 
- IDE integration into the VS code Raspberry Pi Pico extension 
- Sets the target hardware to the original **Raspberry Pi Pico Board**
  - I am not sure if this will also be compatible with the Raspberry Pi Pico 2, or other Raspberry Pis
- Compiles both `main.cpp` (this is where the functionality of the macropad comes from) and `usb_descriptors.c` (this is how the raspberry pi identifies itself to the computer, it is a keyboard and a mouse at the same time)
- Dependencies for the different peripherals are included in here. 
  - I2C is ussed for the OLED display to show the status of the things on screen. 
  - Hardware_pwm: For controlling LED brightness 
  - Hardware_adc: For analog to digital conversion, this is used for the joystick 

The pico_add_extra_outputs is what generates all of the different things that we need to bring into the raspberry pi (including the uf2 file)



