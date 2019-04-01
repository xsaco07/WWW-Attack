import pycurl
import sys
from StringIO import StringIO

GET_ARG = '-u'
FILE_EXT = ""
FILE_NAME = ""

def check_arguments():
    if(len(sys.argv) < 2):
        ARGS_ERROR()
    if(sys.argv[1] != GET_ARG):
        ARGS_ERROR()
    else:
        print("\nARGS INTRODUCED FINE\n")

def ARGS_ERROR():
    print("THE ARGUMENTS INTRODUCED ARE MISSING OR ARE WRONG")
    print(sys.argv)

def download_resource(resource):
    parse_resource(resource)
    print("FILE_NAME: " + FILE_NAME + '.' + FILE_EXT + "\n")
    with open(FILE_NAME + '.' + FILE_EXT, 'wb') as file:
        c = pycurl.Curl()
        c.setopt(c.URL, resource)
        c.setopt(c.WRITEDATA, file)
        c.perform()
        c.close()

def parse_resource(resource):
    period_found = slash_found = False
    period_index = slash_index = 0
    for pos in range(len(resource)-1, -1, -1):
        if(resource[pos] == '.' and not period_found):
            period_index = pos
            period_found = True
        elif(resource[pos] == '/' and not slash_found):
            slash_index =  pos
            slash_found = True
        if(period_found and slash_found):
            break;
    global FILE_EXT, FILE_NAME
    FILE_EXT = resource[period_index+1:]
    FILE_NAME = resource[slash_index+1:period_index]
    print("Resource asked: " + resource + "\n")

def main():
    check_arguments()
    resource = sys.argv[2]
    download_resource(resource)

if __name__ == '__main__':
    main()
