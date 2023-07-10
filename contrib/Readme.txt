Alpao SDK - Linux systems
=========================


CONTENT
-------
   I - LIBRARIES INFORMATION
  II - FOLDERS DESCRIPTION
 III - CONFIGURATION FILES
  IV - INSTALLATION
   V - EXAMPLES
  VI - CONTACT US

 I  - LIBRARIES INFORMATION
---------------------------

 - Generated with:
    System:   CentOS       release 5.10 (Final)
    Compiler: G++ (GCC)    4.1.2 20080704 (Red Hat 4.1.2-54)
    Library:  Glibc-Devel  2.5-118.el5_10.2

 - Minimum requirement:
    GLIBC    - 2.2.5
    GLIBCXX  - 3.4
    GCC      - 3.0

 II - FOLDERS DESCRIPTION
-------------------------
 
 - Drivers: Hardware drivers (Adlink and USB)
 - Lib:     Alpao shared libraries
 - Samples: Program examples

III - CONFIGURATION FILES
-------------------------

You must copy the configuration file from the "Config" folder to your program path.
Or you can copy it anywhere and reference the environment variable ACECFG
 e.g. export ACECFG=/home/myhome/AlpaoCfg/

 IV - INSTALLATION
------------------

 Copy Alpao SDK libraries in your project folder and use the automatic installer "InstallASDK.sh" as root.
 If you need more information on the installation procedure, please contact us at software@alpao.fr

  V - EXAMPLES
--------------

 - GCC
   To compile your program using GCC you must link it with
   the ASDK library, like this:

       gcc example_legacy.c -o Bin/example_legacy -I../../../Include -lasdk 
   or  g++ example.cpp -o Bin/example -I../../../Include -lasdk 

   Depending on your target computer <arch> is "x86" for 32bit or "x64" for 64bit.

 VI - CONTACT US
----------------

 - Website: http://www.alpao.com
 - Mail:    support@alpao.fr