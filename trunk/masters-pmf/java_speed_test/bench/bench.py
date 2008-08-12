runs=10

class Task:
	name = "unnamed"
	pre_cmd = ""
	cmd = ""
	post_cmd = ""
	results = 0
	remain = 1

	def __init__(self):
		self.results = []

tasks = []

t1 = Task()
t1.name = "C version (without print)"
t1.pre_cmd = ""
t1.cmd = "pushd ../c >/dev/null; ./main; popd >/dev/null;"
t1.post_cmd = ""
tasks.append(t1)

t3 = Task()
t3.name = "C version (with print)"
t3.pre_cmd = ""
t3.cmd = "pushd ../c >/dev/null; ./main -p; popd >/dev/null;"
t3.post_cmd = ""
#tasks.append(t3)

t5 = Task()
t5.name = "C version (with print, but sent to /dev/null)"
t5.pre_cmd = ""
t5.cmd = "pushd ../c >/dev/null; ./main -p >/dev/null; popd >/dev/null;"
t5.post_cmd = ""
#tasks.append(t5)

t2 = Task()
t2.name = "Java version (without print)"
t2.pre_cmd = ""
t2.cmd = "pushd ../java >/dev/null; java read_trace; popd >/dev/null;"
t2.post_cmd = ""
tasks.append(t2)

t4 = Task()
t4.name = "Java version (with print)"
t4.pre_cmd = ""
t4.cmd = "pushd ../java >/dev/null; java read_trace -p; popd >/dev/null;"
t4.post_cmd = ""
#tasks.append(t4)

t6 = Task()
t6.name = "Java version (with print, but sent to /dev/null)"
t6.pre_cmd = ""
t6.cmd = "pushd ../java >/dev/null; java read_trace -p >/dev/null; popd >/dev/null;"
t6.post_cmd = ""
#tasks.append(t6)

import os
import time

def average(lst):
	sum = 0
	count = 0

	for i in lst:
		sum += i
		count += 1
	
	if count == 0:
		return 0
	else:
		return sum/count

def min(lst):
	if len(lst) == 0:
		return 0
		
	found = lst[0]

	for i in lst:
		if i < found:
			found = i

	return found

def max(lst):
	if len(lst) == 0:
		return 0
		
	found = lst[0]

	for i in lst:
		if i > found:
			found = i

	return found

def main():
	for task in tasks:
		while task.remain > 0:
			os.system(task.pre_cmd)
			t_start = time.time()
			os.system(task.cmd)
			t_end = time.time()
			os.system(task.post_cmd)
			task.remain-=1
			task.results.append(t_end-t_start)
		print(task.results)

	print "------------------------------------"
	for task in tasks:
		print "RESULTS for " + task.name
		print "Runs: " + str(len(task.results))
		av = average(task.results)
		print "Average run time: " + str(round(av, 3)) + " s"
		print "Rate: " + str(round(1000000.0 / av, 3)) + " events/s"
		#print "Min: " + str(round(min(task.results), 3))
		#print "Max: " + str(round(max(task.results), 3))
		print ""

main()
