# WineWizard
Wine Wizard is a new GUI for Wine written in Qt.

The main feature of Wine Wizard - solutions, that users can create and edit directly from the application. 
Solution lists what libraries, settings and versions of Wine are required for installation and running MS Windows application. If the solution is already created, a user only has to choose the solution and run the installation file, Wine Wizard does the rest of the work automatically. Unlike PlayOnLinux, a knowledge of the Shell is not required for solution creation, everything is done via GUI. Solutions do not require confirmation and verification, which makes it possible to quickly build a database of them. As a last resort, user can write custom script, that will be shown to the user before executing (disabled by default).

Other features:

- Very easy download and installation process for the packages (Winetricks fork), unlimited count of mirrors.
- Wine 32 bit and 64 bit support.
- Advices based on the program logs.
- Automatic creation of sandboxes.
- Different prefixes and versions of Wine for each application.
- Automatic creation, easy manage and deletion of prefixes and application shortcuts.
- Automatic cleaning of temporary files after installation.

***********************************************

Installation of Win-application:

1) Double click on the .exe/.msi file or select "Install Application" from menu.

2) Select solution from list and click "OK".

***********************************************

Dependencies:

qt5, unzip, cabextract, bzip2, tar

***********************************************

Installation (in sources folder):

$ mkdir build

$ cd build

$ qmake ..

$ make

# make install

***********************************************

P.S. System tray icon is hidden by default (tray bug in Qt5 applications) and application is finished automatically. I run Wine Wizard from icon on the KDE panel. You can change the behavior in menu "Settings".
