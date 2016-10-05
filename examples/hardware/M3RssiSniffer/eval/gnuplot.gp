set style data histograms
set style histogram rowstacked
set style fill   solid 1.00 border lt -1
set boxwidth 0.75 absolute
set xtics nomirror norotate

set key outside right center autotitle columnhead

set xlabel 'Channel'
set ylabel 'Channel usage'

set terminal png size 800,600
set output outputFile

plot filename using 2:xtic(1), for [i=3:4] '' using i



