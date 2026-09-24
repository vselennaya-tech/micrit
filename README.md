# micrit
My first project. Minimal Linux init written on C

## What works?
- powering on
- login
- hostnames
- configs
- modules on boot
- 6 tty's
- powering off
- rebooting

## What doesn't work?
- agetty
- PAM
- services
- etc

## Build instructions
```
mkdir bin
gcc -static -v main.c -o ./bin/init
gcc -static -v reboot.c -o ./bin/reboot
gcc -static -v poweroff.c -o ./bin/poweroff
# WARNING! if any other init is installed, command below will overwrite it's needed binaries
sudo cp -v ./bin/* /sbin/
```
