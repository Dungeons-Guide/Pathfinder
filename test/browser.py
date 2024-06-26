import math
import struct
import time
import zlib

f1 = open('test4.1.pfres', 'rb')
f2 = open('test4-local.pfres', 'rb')

world = open('../requests/87bfa2d9-5fe1-494a-9ce7-9ae58709d872.pfreq', 'rb')


file1B = f1.read()
file2B = f2.read()
world = world.read()[0x012C4E:]
worldXLen, worldZLen, worldYLen = struct.unpack('>iii', world[0:12])


nodes1 = file1B[0x012C4B:]
nodes2 = file2B[0x012C4B:]

zLibComp1 = nodes1[4:]
zLibComp2 = nodes2[4:]

decomp1 = zlib.decompress(zLibComp1)
decomp2 = zlib.decompress(zLibComp2)

minX, minY, minZ, xLen, yLen, zLen = struct.unpack('>hhhhhh', decomp1[0:12])

def getPath(x1,y1,z1, decomp, xlen, ylen, zlen):
    i = y1 * xlen * zlen + z1 * xlen + x1
    l = []
    x1, y1, z1, t1, cost1 = struct.unpack('BBBBf', decomp[i * 8 + 12: i* 8 + 20])
    while (t1 != 11 and t1 != 12):
        i = y1 * xlen * zlen + z1 * xlen + x1
        l.append({
            'x': x1 / 2.0,
            'y': (y1+minY) / 2.0,
            'z': z1 / 2.0,
            'cost': cost1,
            'type': t1
        })
        x1,y1,z1,t1,cost1 = struct.unpack('BBBBf', decomp[i * 8 + 12: i* 8 + 20])

    l.append({
            'x': x1 / 2.0,
            'y': (y1+minY) / 2.0,
            'z': z1 / 2.0,
            'cost': cost1,
            'type': t1
    })
    return l

def getBlock(x,y,z):
    i = y * worldXLen * worldZLen + z * worldXLen + x
    block = world[12+i * 2:12+i*2+2]
    return (block[0] != 0 or block[1] != 0)

fromIdx = 2039092
x = 52
y = 124
z = 58



p1 = getPath(x,y,z,decomp1,xLen,yLen,zLen)
p2 = getPath(x,y,z,decomp2,xLen,yLen,zLen)


def slice(fromX, fromZ, toX, toZ, y):
    k = 0
    print(f'y={y}\t'+(toX-fromX)*'v')
    for i in range(fromZ, toZ):
        k += 1
        print(f"{i}\t", end='')
        for x in range(fromX, toX):
            um = False
            if x== 26 and y == 124 and i == 29:
                print('M', end='')
                um = True
                continue
            for pt in p1:
                if math.floor(pt['x']) == x and math.floor(pt['y']) == y and math.floor(pt['z']) == i:
                    print('*', end='')
                    um = True
                    break
            for pt in p2:
                if math.floor(pt['x']) == x and math.floor(pt['y']) == y and math.floor(pt['z']) == i:
                    print('#', end='')
                    um = True
                    break
            if not um:
                if getBlock(x,y,i):
                    print('.', end='')
                else:
                    print(' ', end='')
        print()

print(f'{worldXLen} / {worldYLen} / {worldZLen}')

from curses import wrapper


for i in range(75, 125):
    slice(0,0,64,64,i);

# def main(stdscr):
#     currY = 75
#     # Clear screen
#     while True:
#         stdscr.getkey()
#         stdscr.scrollok(True)
#         # This raises ZeroDivisionError when i == 10.
#         # for i in range(0, 9):
#         #     v = i-10
#         #     stdscr.addstr(i, 0, '10 divided by {} is {}'.format(v, 10/v))
#         # stdscr.
#
#         slice(stdscr, 0, 14,64,64, currY)
#
#         stdscr.refresh()
#         k = stdscr.getkey()
#         if k == 'l':
#             currY -= 1
#         if k == 'o':
#             currY += 1
#
# wrapper(main)
