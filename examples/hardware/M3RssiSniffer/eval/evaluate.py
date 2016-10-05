import sys
import os
import re
import subprocess

if (len(sys.argv)) < 2: 
	print "Usage: python %s <path-to-logfiles>" % sys.argv[0]

logdir = sys.argv[1]


def evaluateFile(lines):
	channelMap = {}
	
	for i in range(27):
		channelMap[i] = {'measurements': 0, 'ownAbove90': 0, 'foreignAbove90': 0, 'other': 0}
	
	for line in lines:
		values = re.split('\s',line)

		while '' in values:
			values.remove('')

		if (len(values) != 7):
			continue

		channel = int(values[6])

		c = channelMap[channel]
		c['measurements'] += int(values[0])
		c['ownAbove90'] += int(values[1])
		c['foreignAbove90'] += int(values[3])
		c['other'] += int(values[5])
	
	return channelMap
		
def makeGnuplot(channelMap):
	result = "Channel\town\tforeign\tnon-802.15.4\tfree\n"; 
	i = 11
	while i <= 26:
		c = channelMap[i]

		if c['measurements'] == 0:
			i += 1
			continue

		own = float(c['ownAbove90']) / c['measurements']
		foreign = float(c['foreignAbove90']) / c['measurements']
		other = float(c['other']) / c['measurements']
		free = 1 - own - foreign - other
		
		result += "%d\t%f\t%f\t%f\t%f\n" % (i, own, foreign, other, free)
		i += 1

	return result
	

	

for filename in os.listdir(logdir):

	if '~' in filename or not os.path.isfile(logdir + '/' + filename):
		continue

	logfile = open(logdir + '/' + filename)
	content = logfile.read()
	logfile.close()

	lines = content.split('\n')
	channelMap = evaluateFile(lines)
	result = makeGnuplot(channelMap)
	if not os.path.exists(logdir + '/eval'):
       		os.makedirs(logdir + '/eval')		
	gpFile = open(logdir + '/eval/gp_' + filename, 'w')
	gpFile.write(result)
	gpFile.close()
	
	infile = logdir + '/eval/gp_' + filename
	outfile = logdir + '/eval/plot_' + filename + '.png'
	plotCmd = ['gnuplot', '-e \"filename=\'%s\'\"' % infile , '-e \"outputFile=\'%s\'\"' % outfile, 'gnuplot.gp']	
	
	cmd = ""
	for part in plotCmd:
		cmd += ' ' + part

	print cmd
	os.system(cmd)
	#process = subprocess.Popen(plotCmd, stdout=subprocess.PIPE)
