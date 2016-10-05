{
    n++;
    sum += $1;
}
END { print sum/n;}
