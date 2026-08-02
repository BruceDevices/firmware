import serial
p='COM3'
try:
    s=serial.Serial(p,115200,timeout=1)
    print('OPEN')
    s.write(b'\r')
    s.timeout=0.5
    d=s.read(80)
    print('BYTES', d)
    s.close()
except Exception as e:
    print('ERROR', e)
