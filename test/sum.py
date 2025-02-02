file = open("../log/log.txt", "r")

result = [0, 0, 0, 0, 0, 0]
cnt = 0

for data in file.read().split("\n"):
    if data == "":
        continue
    result = [result[i] + int(data.split(",")[i]) for i in range(len(result))]
    
print(f'''
atomic_t: {result[0]}
atomic_long_t: {result[1]}
atomic64_t: {result[2]}
refcount_t: {result[3]}
kref: {result[4]}
misc: {result[5]}
''')