# -*- coding: utf-8 -*-

import sys


def parser_struct(file_path):
    file_name = file_path + "//ThostFtdcUserApiStruct.h"
    fd = open(file_name)
    for line in fd.readlines():
        print line,
    fd.close()


def parser_datatype(file_path):
    file_name = file_path + "//ThostFtdcUserApiDataType.h"
    fd = open(file_name)
    for line in fd.readlines():
        #print line,
        line.decode(encoding='utf-8')
        try:
            i = line.index("是一个")
            print i, line
        except:
            i = 10

    fd.close()


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print "Usage: python ctp-h-parser.py <ctp-h-file-path>"
    else:
        parser_datatype(sys.argv[1])
        #parser_struct(sys.argv[1])


    print "End"
