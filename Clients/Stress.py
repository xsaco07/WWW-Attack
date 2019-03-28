import threading
import os
import sys

MAX_ARGS = 6
NUM_THREADS = 0
URL_RESOURCE = ""

def parse_args(args, argv):
  if(arguments_OK(args, argv)):
    global NUM_THREADS, URL_RESOURCE
    NUM_THREADS = argv[2]
    URL_RESOURCE = argv[5]
  else:
    print("ERROR IN ARGUMENTS")

def arguments_OK(args, argv):
  return (args == MAX_ARGS and argv[1] == "-n" and argv[4] == "-u")

def execute_client():
  os.system((parse_client(sys.argv[3]) + " -u " + URL_RESOURCE).format(args=sys.argv))

def parse_client(name):
  client = ""
  if('.' in name):
    client = "python httpclient.py"
  else:
    client = "httpclient"
  return client

def start_threading():
    for i in range(int(sys.argv[2])):
        try:
            current_thread = threading.Thread(name="Client"+str(i), target=execute_client)
            current_thread.start()
        except:
            print("Error creating threads.")

def main():
    parse_args(len(sys.argv), sys.argv)
    start_threading()

if __name__ == '__main__':
    main()
