import random

PARTICLE_COUNT = 5000

for i in range(0, PARTICLE_COUNT):
    a1 = random.uniform(1, 10)
    a2 = random.uniform(0.0, 1.0)
    a3 = random.uniform(0.0, 1.0)
    a4 = random.uniform(-1.0, 1.0)
    a5 = random.uniform(-1.0, 1.0)
    print(a1, a2, a3, a4, a5)
