import board
import busio
i2c = busio.I2C(board.GP2, board.GP3) # Pico SDA1
i2c.try_lock()
i2c.scan()

result = bytearray(1)
i2c.readfrom_into(0x74, result); print(result)

i2c.writeto(0x74, b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
i2c.writeto(0x74, bytearray(b'\xF0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'))