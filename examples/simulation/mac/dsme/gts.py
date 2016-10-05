import re
import numpy
import subprocess

channels = 3
gtSlots = 7
#superframes = 4
superframes = 1

pernode = [0]*10

msf = []
for j in range(0,channels):
    b = []
    for i in range(0,superframes):
        b.append(['']*gtSlots)
    msf.append(b)

tab = open("table.tex",'w')
tab.write("""
\documentclass{article}
\usepackage{tikz}
%\usepackage{tabularx}
\usepackage[active,tightpage]{preview}
\PreviewEnvironment{tikzpicture} 
\\begin{document}""")

def printtable():
    tab.write("""\\begin{tikzpicture}
    \\node{
    \\begin{tabular}{||""")
    for superframe in range(0,superframes):
        for gtSlot in range(0,gtSlots):
            tab.write("p{1cm} |")
        tab.write("|")
    tab.write("""} \hline\n""")
    for channel in range(0,channels):
        tab.write('&'.join([item for sublist in msf[channel] for item in sublist]))
        tab.write("\\\\\hline\n")
    tab.write("""\end{tabular}
    };
    \end{tikzpicture}""")

for line in open("mac.log"):
    m = re.search("alloc ([0-9]+)(.)([0-9]+) ([0-9]+),([0-9]+),([0-9]+)", line)
    if m:
        print m.group(0)
        if m.group(2) == '>':
            msf[int(m.group(6))][int(m.group(4))][int(m.group(5))] = m.group(1)+"$\\to$"+m.group(3)
            pernode[int(m.group(1))] += 1
            printtable()

print msf

tab.write("""
\end{document}
""")

tab.close()

subprocess.call(['pdflatex','table.tex'])

print pernode
