#!/usr/local/bin/python
##/usr/bin/env python

import sys

#XXX hack
def convertnewlines(s):
    #print("convert newlines len(s) = %s" %(len(s)))
    newstr = ""
    i = 0
    while i < len(s):
        if s[i] == '\\' and i < len(s)-1: #if find escape character
            #print ("[%d] hit found, i=%s, s[i] = %c"%(len(s), i, s[i]))
            #print ("i+1 = %s, s[ %s ] = %s" %(i+1, i+1, s[i+1]))
            if s[i+1] == 'r':
                #print "got an r"
                c = '\r' #a return?
                i += 1
            elif s[i+1] == 't':
                c = '\t' # a tab?
                i += 1
            elif s[i+1] == 'n':
                #print "got an N"
                c = '\n' # a newline?
                i += 1
            elif s[i+1] == '\"':
                c = '"' #got a quote
                i += 1
            elif s[i+1] == '\\':
                c = '\\' #got a slash
                i += 1
            else:
                errorMessage = ("[\\%c] is not a valid escape code (found in string [%s])" % (s[i+1], s) )
                raise ValueError(errorMessage)
            #print ("iterating")
        else:
            c = s[i]
        newstr = newstr + c
        i+= 1
    #print ("done loop")
    
    return newstr

 
def obfs( inputStr, flag ):
    #print("orig string:\n%s\n, len = %s" %(inputStr, len(inputStr)))
    inputStr = convertnewlines(inputStr)
    #print("afterconvert string:\n%s\n, len = %s" %(inputStr, len(inputStr)))
    
    charcount = 1; # last char
    invert = "{ "    
    
    if flag == 'n':
        bytesToUse = inputStr.encode( "ASCII" )
    elif flag == 'w':
        bytesToUse = inputStr.encode( "UTF-16LE" )
    
    #print "bytes toUse after rencode: %s" %bytesToUse
    for b in bytesToUse:
        inv = ord(b)
        # 2's complement negate
        inv = ~inv % 0xFF + 1
        invert += ("0x%x, " % inv)
        charcount += 1;
    invert = invert[:-2] + ", 0xFF }"
    #print("invstr:\n%s" %invert)
    #print("**********************DONE")
    return invert, charcount

if __name__ == "__main__":
    outputString = "invert:\n%s\n" % ( obfs(sys.argv[1])[0] )
    #print( outputString )
        
