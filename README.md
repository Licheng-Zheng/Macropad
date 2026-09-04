#### Overview
A custom 9-key mechanical macropad with an OLED display and joystick that is very programmable. It acts as a "Rubber Ducky" which is used to send in commands as if it were a keyboard without relying on any external hardware. 
- Can send multi-character macros entirely on-device without using software like AutoHotkey 
- Key to 81 distinct macro assignments, across 9 different modes 
- OLED display to show the node, joystick position and key presses 
- Under $20 dollars in prorated (I only count a proportion of a cost if I only use a portion of the materials) materials. 
  - If you are looking at this, you probably already have some basic materials at home like wires and raspberry pi picos, so it will likely be significantly cheaper. I only had to spend 5 dollars for this project, and that was because I melted my first raspberry pi. 

<img height="300" alt="image" src="https://github.com/user-attachments/assets/408d7f1b-3fb7-4298-8587-06746134ebc3" /> <img height="300" alt="Screenshot 2026-09-04 133624" src="https://github.com/user-attachments/assets/87e399c7-3231-4513-81a4-1c400494ef1a" />

#### Hardware Limitations 
Because this device is a usb device, they can only send in key presses at the rate that your USB polls. For example, I currently have mine set up to quickly type in a file path that I use frequently, and it takes a couple milliseconds for it to type out. This is due to the fact that USB devices aren't able to paste in things (I think it is due to security reasons). Keyboards can only use the HID protocol, which sends one command every USB polling cycle.  
- To bypass this, you can try using a tool like AutoHotkey, bind one of the keys on the macropad to a key not on the keyboard (F13 to F21), and have it send the command instantly using software. However, in this case, the macropad will only work when that particular software is downloaded on your computer. 
- I personally am ok with the macropad typing everything out, and I would prefer if the macropad didn't rely on a software on my computer to function, so I am just keeping it as is.
- Commercial macropads use software similar to AutoHotkey to send commands, but their softwares are significantly better and have more functionality (opening apps) 

#### Editing the functionality of the Macropad 
1. Changing Keys 
Locate the `MACRO_STRINGS[9][9]` - 9 Lists, each with 9 items. This basically means there are 9 presets, each can have 9 different commands. Mkae sure you are changing it for the right profile.
```
const char* MACRO_STRINGS[9][9] = {
    // Profile 0 (Example: Git Workflow)
    {
        "git status\n", "git add .\n", "git commit -m \"Update\"\n",
        "git push\n",   "git pull\n",  "clear\n",
        "",             "",            ""
    },
```
- You would just type in what string you want the macropad to send in. To press enter, put a `\n` (newline character) after it. 
- You can leave things empty with `""`, and it will just display "UNASSIGNED_KEY" on the OLED screen (nothing gets sent to computer)

2. Changing the Profile Names
There are 9 different possible profiles. 
```
const char* MODE_NAMES[9] = {
    "MODE: EMACS", "MODE: NUMPAD", "MODE: CUSTOM 3", 
    "MODE: CUSTOM 4", "MODE: CUSTOM 5", "MODE: CUSTOM 6", 
    "MODE: CUSTOM 7", "MODE: CUSTOM 8", "MODE: CUSTOM 9"
};
```
You can also change the profile selection in the `MODE_SELECTION_MAP`, but as long as you set up the names properly, this isn't necessary. (also makes it a lot more complicated)

**Chords** 
This is super important, this is how you switch different modes. All three keys must be pressed simultaneously to send teh command. 
- Top row: Recalibrate the joystick 
  - THis polls the joystick 20 times in a couple milliseconds to find where its resting state is at. This then becomes the new "middle" this is helpful if the joystick is slightly off from the middle of `2048`. There is a "deadzone" of 150 where if it is less than 150 away from the middle, no output is registered. 
- Middle row: Turn joystick on or off. 
  - When my soldering wasn't good enough, the joystick would jump around, which would move the mouse around. In case that happens during something important, you can toggle the joystick on or off here. 
- Bottom row: Opens the "Select Profile 1-9" menu, where you can then hit another key to select that particular profile.
- Rightmost column: Turns on the bootloader. 
  - This is essentially you plugging it back in with the bootsel button pressed. It allows it to accept code. 
  - This allows you to upload code without having to open the case again. 
  - Note that this won't work if the raspberry pi pico has crashed (this hasn't happened to me, so not sure what happens next from there)

#### Pinout 
This is how everything needs to be soldered, try not to use much more wire than required, because this makes the case very hard to close. For it to fit in the case that I modelled, headers will need to be removed. 

<img width="320" height="300" alt="PXL_20260902_155708936" src="https://github.com/user-attachments/assets/e7f1e409-9440-45bc-8473-e56d95394758" /> <img width="320" height="300" alt="PXL_20260902_155405590" src="https://github.com/user-attachments/assets/cc4a5ec8-1c4b-46c9-8c99-bf60b213289c" />



| Component | Module Pin | Pico Connection (GPIO) | Pico Physical Pin | Notes |
| --- | --- | --- | --- | --- |
| **OLED Display** | SDA | GP0 | 1 | I2C0 SDA |
|  | SCL | GP1 | 2 | I2C0 SCL |
|  | VCC | 3V3(OUT) | 36 |  |
|  | GND | GND | 38 |The raspberry pi pico has multiple grounds that can be used |
| **Analog Joystick** | VRx (Horz) | GP26 | 31 | ADC Channel 0 |
|  | VRy (Vert) | GP27 | 32 | GP28 can also be used (for this one or VRx) if the pin is damaged. These are the only pins that can be used because the rapsberry pi pico only has 3 analog pins (the pins that can interpret an analog signal) |
|  | VCC / + | 3V3(OUT) | 36 | Do not use 5V VBUS. This will deliver more power than the joystick can handle and will probably lead to either the joystick or the pico board not functioning anymore |
|  | GND / - | GND | 33 |  |
| **Mechanical Keys** | Top Left | GP9 | 12 | Route the second pin of every switch to a common Ground (GND). This is called daisy chaining. |
|  | Top Middle | GP10 | 14 | For keyboards with more keys, matrices are used with one way diodes. Because we only have 9 mechanical switches and our raspberry pi has more than enough GPIO pins, we don't need to do this.  |
|  | Top Right | GP11 | 15 |  |
|  | Middle Left | GP6 | 9 |  |
|  | Center | GP7 | 10 |  |
|  | Middle Right | GP8 | 11 |  |
|  | Bottom Left | GP3 | 5 |  |
|  | Bottom Middle | GP4 | 6 |  |
|  | Bottom Right | GP5 | 7 |  |

#### CAD Files
These were printed out on my ENder 3 V2 uising white PLA. FOr more details on 3D printing these parts, see `3D_printing_information.md`
Here is the link to the onshape file, details for the purpose of each piece is also found in `3D_printing_information.md`: https://cad.onshape.com/documents/84586ac2eb88b3ac68ec06c5/w/5dcf2d6096f851e748bc3f74/e/13d985377aa5300de055461e?renderMode=0&uiState=6a9977ccdc39e3e798f7fdec

The STL_Files is for people who don't want to see the models in Onshape and want to print immediately. Some orientation might be required, see `3D_printing_information.md` for how I oriented it. 

I will not be providing my `.uf2` file because everyones needs for a macropad are different, and it requires modifying the `main.cpp` file in order to change its functionality. I will try to create a guide to compiling it for the first time and changing the code. 
For initial testing, I will include the `.uf2` file with just the numpad enabled. 

#### Troubleshooting Guide 
Some of the harder to fix issues that I encountered while making this: 
1. Keys are not mapped properly
   - If they are all sending commands, it means you didn't solder it to the pin outlined in the pinout table above. Fear not!! You can change it in the software so you don't have to resolder anything. You can map the GPIO pin to the correct key switch in `const uint KEY_PINS[9] = {9, 10, 11, 6, 7, 8, 3, 4, 5}; `. 
   - I recommend going into the numpad mode, pressing all the keys (in order), and make the swaps accordingly. 
2. Joystick keeps on dropping lower and lower
   - This meant that my wires were not properly connected to the pico, so the voltage was not getting sent over properly. 
   - Try reseating your wires (heating it up until liquid, and then letting it cool again)
   - I also replaced my first joystick, not sure if that was the issue, but if the carbon track the joystick uses to send its signal to the raspberry pi is broken, the signal will also not work properly 
3. The case is not closing
   - Try unsoldering the wire, cutting it a bit shorter, and make sure they are grouped with similar cables. (I did it where each row of keys had its own bundle) 
   - Use tape to secure it to the bottom of the macropad so it won't get into the way of the spacers that connect the top and bottom halves. 
4. Raspberry pi port doesn't fit into its hole
   - There is a big overhang right above where the port goes, so there is palstic blocking the port from reaching into the hole. 
   - Use a knife to cut away some of the overhanging plastic to get it to fit. Also, only use two screws to hold the raspberry pi pico in, it is more than enough

There are many more issues that I encountered, but the brain has blocked out the trauma and I can't remember some of the issues anymore. 

#### Bill of Materials
| Item | Qty | Description | Cost |
| --- | --- | --- | --- |
| **Raspberry Pi Pico** | 1 | Standard RP2040 microcontroller | $5.00 |
| **Mechanical Switches** | 9 | MX-style switches (Prorated from an $11 50-pack) | $1.98 |
| **Keycaps** | 9 | Numpad keys scavenged from a standard $11 keycap set | ~$2.00 |
| **Analog Joystick Module** | 1 | Standard 5-pin thumbstick module | $3.00 |
| **OLED Display** | 1 | 0.96" 128x64 I2C OLED (SSD1306) | $3.00 |
| **Wire** | 1 | Spool of 28 AWG wire | $3.00 |
| **Fasteners** | 24 | 12x M2 heat set inserts and 12x M2 screws | Scavenged / Assorted |
| **3D Printer Filament** | 96g | PLA at $17/kg. *(Anglers: 8.96g, Top Plate: 23.04g, Bottom Plate: 63.38g, Test Plate: 0.64g)* | $1.63 |
| **USB Cable** | 1 | Data-capable micro-USB cable (Reused from an old phone) | $0.00 ($4.00 new) |
| **Total Prorated Cost** |  |  | **~$19.61** |

The keycaps can also be 3d printed, the stem might is pretty weak and it definitely won't look as great.
This is the printed one I used first which worked pretty well: https://www.printables.com/model/109679-cherry-mx-low-profile-keycap

Hardware lost along the way, you will be remembered: 
- A raspberry pi pico
  - Melted the bootsel button while trying to solder on the ground wire
- A joystick
  - X kept on jumping around. I thought it was due to the carbon track breaking, but it may have been because my wiring was wrong. 
