set title "Kanalauslastung"
set xlabel "Zeit [s]"
set ylabel "Kanalauslastung [%]

set style data lines
set termoption dashed

plot 	datafile using ($1/1000):(($5)/$2 * 100) linetype 1 title "Fremder Traffic", \
	datafile using ($1/1000):(($3)/$2 * 100) linetype 2 title "Eigener Traffic", \
	datafile using ($1/1000):($7/$2 * 100) linetype 4 title "Anderer Traffic"

pause -1



