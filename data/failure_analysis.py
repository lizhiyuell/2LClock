import numpy as np
import matplotlib.pyplot as plt

filepath = "./failure.txt"
output_path = "./failure_data.txt"

dot = []
boundary1 = []
boundary2 = []
time1 = []
time2 = []
state1 = []
state2 = []

factor = 2000

ofs = open(output_path, "w")

with open(filepath, "r") as f:
    i = 0
    phase1 = -1
    phase2 = -1
    for line in f:
        item = line.strip().split(",")
        data = int(item[0])
        dot.append(data)
        curr_phase1 = int(item[3])
        curr_phase2 = int(item[4])
        state1.append(curr_phase1)
        state2.append(curr_phase2)
        time1.append(int(item[1]))
        time2.append(int(item[2]))
        if curr_phase1!=phase1:
            boundary1.append(time1[-1])
            phase1 = curr_phase1
#             if curr_phase1!=0:
#                 print("TS change in phase{0} -> phase{1}: {2}".format(phase1-1, phase1, time1[i]-time1[i-1]))
        if curr_phase2!=phase2:
            boundary2.append(time1[-1])
            phase2 = curr_phase2
#             if curr_phase2!=0:
#                 print("TS change in phase{0} -> phase{1}: {2}".format(phase2-1, phase2, time2[i]-time2[i-1]))
        i+=1


dot = np.array(dot)
upper = np.max(dot)
lower = np.min(dot)

yy = np.arange(lower, upper, 200)
xx = np.ones(np.shape(yy))


# for x in boundary1:
#     plt.plot(x*xx/factor, yy, color="C3")
# for x in boundary2:
#     plt.plot(x*xx/factor, yy, color="C2")
    
# # select a half
# x = np.arange(0, 50000, 1)
# x_aixs = x_aixs[0:35000:50]
# dot = dot[0:35000:50]
# x = x[0:35000:50]  
# x_aixs = (np.array(time1)-time1[0])/1e9
# plt.scatter(x_aixs, dot, s=1)
# plt.show()

# -------------------------------

boundary1 = (np.array(boundary1) - time1[0])/1e9
boundary2 = (np.array(boundary2) - time1[0])/1e9

x_aixs = (np.array(time1)-time1[0])/1e9
x = np.arange(0, 50000, 1)

sub_x = x_aixs[0:35000:50]
sub_dot = np.abs(dot[0:35000:50])

x = x[0:35000:50]/factor
x_aixs = (np.array(time1)-time1[0])/1e9

plt.scatter(sub_x, sub_dot, s=1)


for x in boundary1:
    plt.plot(x*xx, np.abs(yy), color="C3")
for x in boundary2:
    plt.plot(x*xx, np.abs(yy), color="C2")
# plt.show()
plt.savefig("failure.png")


# output data
for n in range(len(x_aixs)):
    ofs.write("{0} {1}\n".format(x_aixs[n], abs(dot[n])))
ofs.close()

