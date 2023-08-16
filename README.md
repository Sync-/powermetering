# powermetering
```
ADE9000 application circuit based power meter
default ip: 192.168.178.204
st-flash --reset write build/powermetering.bin 0x08020000
```

Building:
```
cd software/website
./makefsdata
./copy.sh

cd software/powermetering
make
```

Flashing:
```
socat TCP:192.168.178.204:2000 build/powermetering.bin
```
