import os
import stat
import time

runs=10

class Task:
	name = "unnamed"
	pre_cmd = ""
	cmd = ""
	post_cmd = ""
	results = 0
	remain = 3

	def __init__(self):
		self.results = []

	def print_extra_details(self):
		pass


class JavaTestTask(Task):
	tracefile = ""

	def print_extra_details(self):
		size = os.stat(self.tracefile)[stat.ST_SIZE]
		tot_events = size / 22
		print "Tracefile: %s (%d bytes)" % (self.tracefile,size)
		print "Events in tracefile: %d" % tot_events
		print "Rate: " + str(round(float(tot_events) / self.average_run_time, 3)) + " events/s"


tasks = []

t1 = JavaTestTask()
t1.name = "C version (without print)"
t1.tracefile = "../trace_long.dat"
t1.pre_cmd = ""
t1.cmd = "pushd ../c >/dev/null; ./main %s; popd >/dev/null;" % t1.tracefile
t1.post_cmd = ""
tasks.append(t1)

t3 = JavaTestTask()
t3.name = "C version (with print)"
t3.tracefile = "../trace_med.dat"
t3.pre_cmd = ""
t3.cmd = "pushd ../c >/dev/null; ./main -p %s; popd >/dev/null;" % t3.tracefile
t3.post_cmd = ""
tasks.append(t3)

t5 = JavaTestTask()
t5.name = "C version (with print, but sent to /dev/null)"
t5.tracefile = "../trace_long.dat"
t5.pre_cmd = ""
t5.cmd = "pushd ../c >/dev/null; ./main -p %s >/dev/null; popd >/dev/null;" % t5.tracefile
t5.post_cmd = ""
tasks.append(t5)

t2 = JavaTestTask()
t2.name = "Java version (without print)"
t2.tracefile = "../trace_long.dat"
t2.pre_cmd = ""
t2.cmd = "pushd ../java >/dev/null; java read_trace %s; popd >/dev/null;" % t2.tracefile
t2.post_cmd = ""
tasks.append(t2)

t4 = JavaTestTask()
t4.name = "Java version (with print)"
t4.tracefile = "../trace_short.dat"
t4.pre_cmd = ""
t4.cmd = "pushd ../java >/dev/null; java read_trace -p %s; popd >/dev/null;" % t4.tracefile
t4.post_cmd = ""
tasks.append(t4)

t6 = JavaTestTask()
t6.name = "Java version (with print, but sent to /dev/null)"
t6.tracefile = "../trace_med.dat"
t6.pre_cmd = ""
t6.cmd = "pushd ../java >/dev/null; java read_trace -p %s >/dev/null; popd >/dev/null;" % t6.tracefile
t6.post_cmd = ""
tasks.append(t6)

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
		task.average_run_time = average(task.results)
		print "Average run time: " + str(round(task.average_run_time, 3)) + " s"
		task.print_extra_details()
		#print "Min: " + str(round(min(task.results), 3))
		#print "Max: " + str(round(max(task.results), 3))
		print ""

main()
