import numpy as np
import time

def writeMatrix(mat, name):
    s = mat.shape
    with open(name, "w") as f:
        f.write(f"{s[0]}x{s[1]}")
        f.write("\n")
        for r in mat:
            f.write(f",".join([str(c) for c in r]))
            f.write("\n")

def countLineChars(name):
    lines = open(name, "r").readlines()
    m = max(lines, key=lambda x: len(x))
    return len(m)

n = 57
m = 21
p = 41
A = np.random.randint(128, size=(n, m), dtype=np.uint16)
writeMatrix(A, "A.dat")
B = np.random.randint(128, size=(m, p), dtype=np.uint16)
writeMatrix(B, "B.dat")

C = np.matmul(A, B)
writeMatrix(C, "C.dat")

print("Max line char count in A.dat", countLineChars("A.dat"))
print("Max line char count in B.dat", countLineChars("B.dat"))
print("Max line char count in C.dat", countLineChars("C.dat"))
