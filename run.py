import os
import sys

def main():
	cmd_clear=">result_withIO.txt"
	os.system(cmd_clear)
	cmd_clear=">result_withoutIO.txt"
	os.system(cmd_clear)
	iteration_times = [10, 20, 50, 100, 200, 500, 1000, 2000, 5000]
	#for file with io
	filename = "result_withIO.txt"
	for num in iteration_times:
		#filename = "result_io_"+str(num)+".csv"
		cmd = "taskset -c 1 ./simple_io.x " +str(num)+"> temp.txt"
		os.system(cmd)
		with open(filename,'a') as outfile:
			with open("temp.txt") as infile:
				content = infile.read()
				outfile.write(content)
		

	#for file without io
	filename2 = "result_withoutIO.txt"
	for num in iteration_times:
		#filename = "result_noio_"+str(num)+".csv"
		cmd = "taskset -c 1 ./simple_noio.x " +str(num)+"> temp.txt"
		os.system(cmd)
		with open(filename2,'a') as outfile:
			with open("temp.txt") as infile:
				content = infile.read()
				outfile.write(content)

if __name__=='__main__':
	main()
