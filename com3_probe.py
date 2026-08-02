import serial
ports=['COM3','COM4','COM5','COM6']
for p in ports:
    try:
        s=serial.Serial(p,115200,timeout=1)
        print(p, 'OPEN')
        s.write(b'\r')
        s.timeout=0.5
        try:
            data=s.read(80).decode('utf-8','ignore')
            print('DATA:', data.replace('\n','\\n'))
        except Exception as e:
            print('READ_ERR', e)
        s.close()
    except Exception as e:
        print(p, 'CLOSED', e)
