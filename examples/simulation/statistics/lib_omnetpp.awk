@include "lib_eval.awk";


# parses the line of an omnetpp logfile which contains 
# the named iteration variables
# 
# Expressions contained in the value part of each
# name-value pair will be parsed and evaluated 
#
# Names and values are stored with the same index in the 
# corresponding arrays. Order of their appearance is 
# maintained
# 
# Resulting arrays start with index 1
#
# Input parameters:
#    line - the line containing the named iteration variables
#    nArray - array which will contain the names of the parsed data
#    vArray - array which will contain the values of the parsed data 
# 
# Returns the number of name value pairs
function parseIterationVarsLine (line, nArray, vArray) {    
    # extract the name-value string from line
    #s = split(line, parts, "\"");
    # search for first occurrence of quotation marks
    start = index(line, "\"")
    line = substr(line, start+1)
    line = substr(line, 1, length(line)- 1)
        
    
    # second part contains the actual string
    nvString=line
        
    # now split it again using the "," separator
    numFields=split(nvString,fields, ",");
    for (j = 1; j <=numFields; j++) {
        split(fields[j], nameValuePair, "=");
        name = nameValuePair[1]
        value = nameValuePair[2]
        
        # remove "$ or $ from beginning of attribute name
        gsub(/(\$|"|[[:space:]])/, "", name);
        nArray[j] = name;

        # check if parameter matches 'true' or '\"<param>\"' -- this hints at a string or
        # bool parameter and should not be evaluated
        if ((value ~ /\\\".*\\\"/) || (value ~ /true/) || (value ~ /false/)) { 
            gsub(/\\\"/, "\"", value)
            vArray[j] = value
        }
        else {
            # assign value, which can be an algebraic expression,
            # name-value array, but first remove all letters (units) and trailing '"'
            gsub(/[[:alpha:]]|\"/, "", value);
            params["@error_fatal"] = 1        
            
            # use expression evaluation lib to evaluate algebraic expression
            vArray[j] = eval(value, params);
        }
    }
    
    return j-1;
}
