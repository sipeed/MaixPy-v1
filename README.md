Micropython based on the Sipeed Maix one platform
===============================================

<p align="center">
  <img src="http://i1.bvimg.com/666569/516250f0d8a0716d.jpg" alt="Sipeed Maix one"/>
</p>


MaixPy Introduction
---------------------

MaixPy is an open source MicroPython project based on the Sipeed M1 platform. It supports a variety of peripherals on the Sipeed M1. It is designed to make programming easier.It is  based on the K210-standalone-sdk.

MaixPy version is Python 3.4

For more information on MaixPy, please visit http://maixpy.sipeed.com/


Directory architecture
---------------------------

MaixPy directory architecture:

- py/ -- a core Python implementation that includes the compiler, runtime, and core libraries.

- mpy-cross/ --  the MicroPython cross-compiler which is used to turn scripts into precompiled bytecode.

- ports/k210-standalone/ -- micropython porting code based on k210 platform sdk

- tests/ -- test framework and test scripts

- docs/ -- user documentation in Sphinx reStructuredText format. Rendered HTML documentation is available at http://docs.micropython.org (be sure to select needed board/port at the bottom left corner).

Platform porting code directory architecture:

- board-drivers/ store the onboard module driver code

- buildin-py/ firmware built-in microPython script

- mpy-mod/ micropython module code

- spiffs/ spiffs file system source code

- spiffs-port/ spiffs porting configuration code

- kendryte-standalone-sdk/ k210 sdk generated after using the build script

Build and compile
--------------------------------------------

Build code:

	$ git clone git@github.com:sipeed/MaixPy.git #Download sdk
	$ cd port/k210-standalon/ #Enter the platform code directory
	$ make build #build platform code in case of first use

Compile the code:

	$ make CROSS_COMPILE=/your_compiler_path


`your_compiler_path` is the compiler path, about the compiler , you can see http://dan.lichee.pro/

After compiling, the micropython.bin file will be generated in this directory, and you can burne it to the Sipeed Maix One suite. 

The burning method can be found at http://dan.lichee.pro/

contribution
------------

MaixPy is an open source project and welcomes contributions. MicroPython is licensed under the MIT license and all contributions are subject to this license.


























