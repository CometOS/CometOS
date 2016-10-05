function postprocessing {
	if [ $#  -ne 8 ]; then 
		echo "required parameters: <root_folder> <xkey> <xvalue:array> <ykey> <yvalueoffset> <curvekey> <curvevalue:array> <filter:name assoicative array>";
		return; 
	fi

	ROOT=$1
	#echo "# root=$ROOT"
	#FILE="/dev/stdin"
	#echo "# file=$FILE"
	XKEY=$2
	#echo "# xkey=$XKEY"
	XVALUE=$3
	#for i in "${XVALUE[@]}"; do echo "# xvalues "$i; done
	YKEY=$4
	#echo "# ykey=$YKEY"
	YVALUEOFFSET=$5
	#echo "# yvalueoffset=$YVALUEOFFSET"
	CURVEKEY=$6
	#echo "# curvekey=$CURVEKEY"
	CURVEVALUE=$7
	#for i in "${CURVEVALUE[@]}"; do echo "# curves "$i; done

	e="$( declare -p $8 )"
   	eval "declare -A INCLUDEFILTER=${e#*=}"
	


	AWK_STATS=$ROOT/statistics/basic.awk
	AWK_SELECT=$ROOT/statistics/select.awk
	AWK_CUT=$ROOT/statistics/cut.awk

	

	# create temporary file
	temp=$(mktemp)
	cat /dev/stdin >$temp


	# step 1 filtering of data
	for k in "${!INCLUDEFILTER[@]}"
	do
		# echo "# execute filter $k=${INCLUDEFILTER[$k]}"
		temp2=$(mktemp)
		cat $temp | awk -v key=$k -v value=${INCLUDEFILTER[$k]} -f $AWK_SELECT > $temp2
 		temp=$temp2;	
	done

	header="$XKEY "
	for curve in "${CURVEVALUE[@]}"; do 
		header=$header" n-"$curve" "$curve" err-"$curve 
	done
	echo $header

	
	# step 2 extract x data
	for value in "${XVALUE[@]}"; do 
		tempX=$(mktemp)
		cat $temp | awk -v key=$XKEY -v value=$value -f $AWK_SELECT > $tempX

		result="$value ";

		# step 3 select curve and merge data
		for curve in "${CURVEVALUE[@]}"; do 
			result=$result" "$(cat $tempX | awk -v key=$CURVEKEY -v value=$curve -f $AWK_SELECT | awk -v key=$YKEY -v offset=$YVALUEOFFSET -f $AWK_CUT | awk -f $AWK_STATS | cut -d " " -f 2,4,6)
		done

		echo $result

		# write data

		
	done

} 


function createTex {
	if [ $#  -ne 4 ]; then 
		echo "required parameters: <x label> <y label> <title> <curves>";
		return; 
	fi
	
	XLABEL=$1
	echo "% xlabel $XLABEL"
	YLABEL=$2
	echo "% ylabel $YLABEL"
	TITLE=$3	
	e="$( declare -p $4 )"
   	eval "declare -A CURVES=${e#*=}"
   	
   	temp=$(mktemp)
	cat /dev/stdin >$temp
		
	body=""

	for label in "${!CURVES[@]}"
	do
		offset=${CURVES[$label]}
		body=$body"\plotFileWithError{$temp}{$offset}"

		#body=$body" \addplot+[error bars/.cd,y dir=both,error mark=triangle*,y explicit] table[x index=0,y index=$offset,y error index=$(($offset+1)),header=true] {$temp};"
		#body=$body" \addlegendentry{$label};"
	done
	
	
# print tex file		
cat  << EOF

\documentclass[10pt,a4paper,parskip]{scrbook}
\usepackage[T1]{fontenc}
\usepackage{textcomp}
\usepackage[scaled=.92]{helvet}
\usepackage{courier}
\usepackage{amsmath}
\usepackage{amssymb}
\usepackage{pifont}
\usepackage[nomessages]{fp}
\usepackage{tikz}
\usepackage{pgfplots}
\usetikzlibrary{shapes,shadows,arrows,patterns,trees,automata,positioning,snakes}
\usetikzlibrary{decorations.text}
\usetikzlibrary{decorations.fractals}
\usetikzlibrary{decorations.markings}
\usetikzlibrary{decorations.pathmorphing}
\usetikzlibrary{decorations.text}
\usetikzlibrary{decorations.footprints}
\usetikzlibrary{decorations.pathreplacing}
\usetikzlibrary{decorations.shapes}
\usetikzlibrary{calc}
\usepackage[free-standing-units]{siunitx}
\usepackage{tuhhcolor}
\usepackage[active,tightpage]{preview}
\PreviewEnvironment{tikzpicture}
\pagestyle{empty}


\newcommand{\plotFileWithError}[2]{
\pgfplotstableread{#1}{\table}
\pgfplotsforeachungrouped \algNum in {#2}{
	\FPeval{\nameColumn}{clip(2+3*\algNum)}%
	\FPeval{\dataColumn}{clip(2+3*\algNum)}%
	\FPeval{\errorColumn}{clip(3+3*\algNum)}%
   	\pgfplotstablegetcolumnnamebyindex{\nameColumn }\of{\table}\to{\colname}
   	\addplot table[x index=0, y expr=\thisrowno{\dataColumn},y error expr=\thisrowno{\errorColumn}] {#1};
     \addlegendentryexpanded{\colname}
}
}


\pgfplotsset{LINE_STYLE_0/.style={%
	mark options={fill=purple!70!black,mark size=1.6},
	draw=purple!40!black,line width=0.5pt, %dotted,
	error bars/y dir=both,
	error bars/y explicit,
	error bars/error mark options={rotate=90, color=purple!70!black, mark size=2.5pt, line width=0.5pt},
	mark=x
	}
}


\pgfplotsset{LINE_STYLE_1/.style={%
	mark options={fill=green!70!black,mark size=1.4},
	draw=green!40!black,line width=0.5pt, %dotted,
	error bars/y dir=both,
	error bars/y explicit,
	error bars/error mark options={rotate=90, color=green!70!black, mark size=2.5pt, line width=0.5pt},
	mark=square*
	}
}     
              
\pgfplotsset{LINE_STYLE_2/.style={%
	mark options={fill=red!70!black,mark size=1.6},
	draw=red!40!black,line width=0.5pt, %dotted,
	error bars/y dir=both,
	error bars/y explicit,
	error bars/error mark options={rotate=90, color=red!70!black, mark size=2.5pt, line width=0.5pt}, 
	mark=triangle*
	}
}     

\pgfplotsset{LINE_STYLE_3/.style={%
	mark options={fill=blue!70,mark size=1.4},
	draw=blue!40!black,line width=0.5pt, %dotted,
	error bars/y dir=both,
	error bars/y explicit,
	error bars/error mark options={rotate=90, color=blue!70, mark size=2.5pt, line width=0.5pt},
	mark=*}
}

\pgfplotsset{LINE_STYLE_4/.style={%
	mark options={fill=orange!70!black,mark size=1.6},
	draw=orange!40!black,line width=0.5pt, %dotted,
	error bars/y dir=both,
	error bars/y explicit,
	error bars/error mark options={rotate=90, color=orange!70!black, mark size=2.5pt, line width=0.5pt},
	mark=diamond*}
}



\pgfplotsset{LINE_STYLE_11/.style={%
	mark options={fill=green!70!black,mark size=1.4},
	draw=green!40!black,line width=0.5pt, %dotted,
	error bars/y dir=both,
	error bars/y explicit,
	error bars/error mark options={rotate=90, color=green!70!black, mark size=2.5pt, line width=0.5pt},
	mark=square}
}     
              
\pgfplotsset{LINE_STYLE_12/.style={%
	mark options={fill=red!70!black,mark size=1.6},
	draw=red!40!black,line width=0.5pt, %dotted,
	error bars/y dir=both,
	error bars/y explicit,
	error bars/error mark options={rotate=90, color=red!70!black, mark size=2.5pt, line width=0.5pt},
	mark=triangle}
}     

\pgfplotsset{LINE_STYLE_13/.style={%
	mark options={fill=blue!70,mark size=1.4},
	draw=blue!40!black,line width=0.5pt, %dotted,
	error bars/y dir=both,
	error bars/y explicit,
	error bars/error mark options={rotate=90, color=blue!70, mark size=2.5pt, line width=0.5pt},
	mark=o}
}

\pgfplotsset{LINE_STYLE_14/.style={%
	mark options={fill=orange!70!black,mark size=1.6},
	draw=orange!40!black,line width=0.5pt, %dotted,
	error bars/y dir=both,
	error bars/y explicit,
	error bars/error mark options={rotate=90, color=orange!70!black, mark size=2.5pt, line width=0.5pt},
	mark=diamond}
}


\pgfplotscreateplotcyclelist{default-error-plot}{%
LINE_STYLE_0\\\\%
LINE_STYLE_1\\\\%
LINE_STYLE_2\\\\%
LINE_STYLE_3\\\\%
LINE_STYLE_4\\\\%
LINE_STYLE_11\\\\%
LINE_STYLE_12\\\\%
LINE_STYLE_13\\\\%
}

\begin{document}


\begin{tikzpicture}

\begin{axis}[
cycle list name=default-error-plot,
legend style={at={(0.0,1.0)},anchor=north west,draw=none},
grid=major,
ylabel={$YLABEL},
xlabel= {$XLABEL},
title= {$TITLE},
]
$body
\end{axis}
\end{tikzpicture}
\end{document}
EOF

}


