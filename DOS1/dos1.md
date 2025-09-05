## Building the Malware

### A. Building the C++ Service (PowerfakeShell)

- On your Analysis VM, open x64 Native Tools Command Prompt for VS 2022.

- Navigate to the directory containing dos1.cpp.

- Compile the code using the following command:

```Powershell
cl /EHsc /Fe:PowerfakeShell.exe dos1.cpp /link /SUBSYSTEM:WINDOWS
```

- This will generate PowerfakeShell.exe, the persistent service binary.

### B. Preparing the Python Component

- Ensure Python is installed on the Analysis VM.
- Install the required library:

```bash
pip install pycryptodome
```

- The Python script (dos1.py) does not need compilation. It will be run directly by the Python interpreter.


## Deployment & Execution

### Transfer Files to Victim VM

- Copy the following files from your Analysis VM to the Desktop of your Victim VM (e.g., using a VirtualBox Shared Folder or by dragging and dropping):

    - PowerfakeShell.exe (The C++ compiled binary)
    - dos1.py (The Python script)

### Install the Persistent Service (C++)

- On the Victim VM, open a Command Prompt as Administrator.

- Navigate to the Desktop and run the installer command:

```bash 
cd C:\Users\Victim\Desktop
PowerfakeShell.exe --install
```

- This will install the malicious Windows service named PowerfakeShell and start it automatically. The service is now persistent and will survive reboots.

### Execute the Python Componen

- In the same Command Prompt (still as Admin), run the Python script to initiate the immediate resource exhaustion and scheduled BSOD:
```bash
python dos1.py --persist
```
- The --persist argument will also add a registry entry to run the Python script on user login.
