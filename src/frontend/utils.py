from termcolor import colored, cprint
import subprocess
from subprocess import call

def info(message):
    cprint('Info: ' + message, attrs=['bold'])

def warn(message):
    cprint('Warning: ' + message, 'cyan', attrs=['bold'])

def error(message):
    cprint('Error: ' + message, 'red', attrs=['bold'])

def log_from_iterator(lines_iterator, logfile):
  with open(logfile, 'a') as log:
    for line in lines_iterator:
      log.write(line)
      log.flush()

def print_and_log(logfile, line):
  with open(logfile, 'a') as log:
    log.write(line)
    log.flush()
  print line

def log(logfile, line):
  with open(logfile, 'a') as log:
    log.write(line)
    log.flush()

def execute(command, logfile):
  err_logfile = logfile + '.err'

  command_str = ' '.join(command)
  log(logfile, 'Executing:     {}'.format(command_str))
  log(logfile, '  log:         {}'.format(logfile))
  log(logfile, '  errlog:      {}'.format(err_logfile))

  popen = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  log_from_iterator(iter(popen.stdout.readline, b"") , logfile)
  log_from_iterator(iter(popen.stderr.readline,  b""), err_logfile)
  popen.wait()

  retcode = str(popen.returncode)

  if popen.returncode != 0:
    print colored('Error! "{}" return code {}'.format(command_str, retcode), 'red')

  return retcode
