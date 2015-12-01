# bbblack
Follow link to create a bootable sdcard for beagle bone black
https://eewiki.net/display/linuxonarm/BeagleBone+Black

Clone u-boot repo, patch it.

Modify the am33xx.dtsi file timer7 .compatible string to "sk,timer-dev".
This should match the driver .compatible string. Failure to modify the string will result in the driver's probe function not being called and therefore none of it will work


lkml/dmtimer is a kernel module where an interrupt is fired every one sec using  DMTIMER 7.
