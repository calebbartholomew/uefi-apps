UEFI Applications
=================

This repo contains UEFI applications written by Caleb Bartholomew.

## Applications

### DNSCLient
A simple DNS client that can be executed out of the firmware.  Some known issues exist, read DNSClient.inf for more info.

## Compiling
* Symlink or hardlink CabAppPkg into the edk2 folder
* Change ACTIVE_PLATFORM to CabAppPkg/CabAppPkg.dsc inside target.txt
* Setup EDK environment variables and source edksetup.sh
* Execute the build command.
