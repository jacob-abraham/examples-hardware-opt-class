import numpy as np
import time

def writePoints(points):
    with open("points.dat", "w") as f:
        f.write(f"{len(points)}\n")
        for point in points:
            f.write(f",".join([str(p) for p in point]))
            f.write("\n")

def writeExpected(point):
    with open("exp.dat", "w") as f:
        f.write(f",".join([str(p) for p in point]))
        f.write("\n")

n = 10000
MAX = 2**15
x = np.random.randint(-(MAX- 1), MAX, size=n, dtype=np.int32)
y = np.random.randint(-(MAX - 1), MAX, size=n, dtype=np.int32)
z = np.random.randint(-(MAX- 1), MAX, size=n, dtype=np.int32)

points = list(zip(x,y,z))
centorid = (int(np.mean(x)), int(np.mean(y)), int(np.mean(z)))

writePoints(points)
writeExpected(centorid)


