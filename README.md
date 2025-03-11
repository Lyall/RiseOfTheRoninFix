# RiseOfTheRoninFix
[![Patreon-Button](https://github.com/Lyall/RiseOfTheRoninFix/blob/main/.github/Patreon-Button.png?raw=true)](https://www.patreon.com/Wintermance) 
[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/W7W01UAI9)<br />
[![Github All Releases](https://img.shields.io/github/downloads/Lyall/RiseOfTheRoninFix/total.svg)](https://github.com/Lyall/RiseOfTheRoninFix/releases)

**RiseOfTheRoninFix** is an ASI plugin for Rise of the Ronin that:
- Adds support for custom resolutions.
- Fixes cropped FMVs at ultrawide resolutions.

## Installation  
- Download the latest release from [here](https://github.com/Lyall/RiseOfTheRoninFix/releases). 
- Extract the contents of the release zip in to the the game folder.  

### Steam Deck/Linux Additional Instructions
🚩**You do not need to do this if you are using Windows!**  
- Open the game properties in Steam and add `WINEDLLOVERRIDES="dinput8=n,b" %command%` to the launch options.  

## Configuration
- Open **`RiseOfTheRoninFix.ini`** to adjust settings.

## Screenshots
| ![ezgif-8ed5bbb9dca544](https://github.com/user-attachments/assets/99083863-1ea5-4cd8-92fd-102a604f37b1) |
|:--:|
| Fixed FMV Cropping |

## Credits
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[spdlog](https://github.com/gabime/spdlog) for logging. <br />
[safetyhook](https://github.com/cursey/safetyhook) for hooking.
