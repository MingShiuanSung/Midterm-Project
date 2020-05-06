import numpy as np
import serial
import time


serdev = '/dev/ttyACM0'

s = serial.Serial(serdev)

waitTime = 0.01


class song:
  def __init__(self, name, num, note, length):
    self.name = name
    self.num = num
    self.note = note
    self.length = length

twinkel = song(
    'Twinkel Twinkel little star',

    42, 

    [261, 261, 392, 392, 440, 440, 392,

    349, 349, 330, 330, 294, 294, 261,

    392, 392, 349, 349, 330, 330, 294,

    392, 392, 349, 349, 330, 330, 294,

    261, 261, 392, 392, 440, 440, 392,

    349, 349, 330, 330, 294, 294, 261], 

    [1, 1, 1, 1, 1, 1, 2,

    1, 1, 1, 1, 1, 1, 2,

    1, 1, 1, 1, 1, 1, 2,

    1, 1, 1, 1, 1, 1, 2,

    1, 1, 1, 1, 1, 1, 2,

    1, 1, 1, 1, 1, 1, 2])

birthday = song(
  'Happy Birthday',

  25,

  [ 261, 261, 293, 261, 349, 329,

    261, 261, 293, 261, 391, 349,

    261, 261, 523, 440, 349, 329,

    293, 466, 466, 440, 349, 392,

    349],

  [ 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1,
    1])

MacDonald = song(
  'Old MacDonald had a farm',

  45,

  [523, 523, 523, 392, 440, 440,

  392, 659, 659, 587, 587, 523,

  391, 523, 523, 523, 391,
  
  440, 440, 391, 659, 659,
  
  587, 587, 523, 391, 523,
  
  523, 523, 391, 523, 523, 523,
  
  523, 523, 523, 391, 440,
    
  440, 391, 659, 659, 587, 587,
  
  523],

  [1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  1, 1, 1])

songTable = [twinkel, birthday, MacDonald]

flag = False

while(True):
  line = s.readline()

  if line == b'read song list\r\n':  # input command is 'read song list'
    print("sending list...")
    s.write(bytes(str(len(songTable)), 'UTF-8'))
    s.write(b'-')

    for data in songTable:
      s.write(bytes(data.name, 'UTF-8'))
      s.write(b'-')
      print(data.name)

    print("list sended")

  for data in songTable:
    if line == bytes(data.name + '\r\n', 'UTF-8'):
      print("sending song...")

      time.sleep(1)

      s.write(bytes(data.name, 'UTF-8'))

      s.write(b'-')

      s.write(bytes(str(data.num), 'UTF-8'))

      s.write(b'-')

      for datas in data.note:
        s.write(bytes(str(datas), 'UTF-8'))
        s.write(b',')
        time.sleep(waitTime)

      s.write(b'-')

      for datas in data.length:
        s.write(bytes(str(datas), 'UTF-8'))
        s.write(b',')
        time.sleep(waitTime)

      s.write(b'-')

      for datas in data.length:
        s.write(bytes(str(datas), 'UTF-8'))
        s.write(b',')
        time.sleep(waitTime)

      s.write(b'-')




      print("song sended")




# write data functions
'''
print("Sending signal ...")

for data in songTable:
  s.write(bytes(data.name, 'UTF-8'))

s.write(bytes(songTable[1].name, 'UTF-8'))

s.write(b'-')

s.write(bytes(str(songTable[0].num), 'UTF-8'))

s.write(b'-')

for data in songTable[0].note:
  s.write(bytes(str(data), 'UTF-8'))
  s.write(b',')
  time.sleep(waitTime)

s.write(b'-')

for data in songTable[0].length:
  s.write(bytes(str(data), 'UTF-8'))
  s.write(b',')
  time.sleep(waitTime)

s.write(b'-')

s.close()
print("Signal sended")
'''

