from termcolor import colored, cprint
import subprocess
from subprocess import call

def info(message):
    cprint('Info: ' + message, attrs=['bold'])

def warn(message):
    cprint('Warning: ' + message, 'cyan', attrs=['bold'])

def error(message):
    cprint('Error: ' + message, 'red', attrs=['bold'])

def print_from_iterator(lines_iterator, logfile=None):
  output = ''
  if logfile:
    with open(logfile, 'w') as log:
      for line in lines_iterator:
        log.write(line)
        log.flush()
        output += line
  else:
    for line in lines_iterator:
      print line
      output += line
  return output

def execute(command, logfile=None, silent=False):
  if not silent:
    print 'Executing ', ' '.join(command)
  popen = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

  output = None
  output_err = None
  if logfile or not silent:
    output = print_from_iterator(iter(popen.stdout.readline, b"") , logfile)
    err_log = logfile + '.err' if logfile else None
    output_err = print_from_iterator(iter(popen.stderr.readline,  b""), err_log)

  popen.wait()
  if popen.returncode != 0:
    if logfile:
      print 'log: ', logfile
      print 'errlog:  ', logfile + '.err'
    print colored("Error! '{0}' return code {1}".format(
      ' '.join(command), str(popen.returncode)) ,
      'red')

  return output, output_err
