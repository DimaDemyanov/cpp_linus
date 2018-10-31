#include <syslog.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <fstream>
#include <sstream> 

void sig_handler(int signo)
{
  if(signo == SIGTERM)
  {
    syslog(LOG_INFO, "SIGTERM has been caught! Exiting...");
    if(remove("run/daemon.pid") != 0)
    {
      syslog(LOG_ERR, "Failed to remove the pid file. Error number is %d", errno);
      exit(1);
    }
    exit(0);
  }
}

void handle_signals()
{
  if(signal(SIGTERM, sig_handler) == SIG_ERR)
  {
    syslog(LOG_ERR, "Error! Can't catch SIGTERM");
    exit(1);
  }
}

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    //if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            result += buffer.data();
    }
    return result;
}


void run_tool(){
  //printf("asdas");
  system("sudo dmesg -C");
  system("sudo insmod run/lkm_hello1.ko");
  system("sudo dmesg -t > pids_all");
  system("sudo rmmod run/lkm_hello1.ko");
  system("ps -Ao pid | awk '{ print $1}' > pids_ps");
  system("sed '1d' pids_ps > pids_ps1");
  //system("echo '555' > pids_ps");
  //sleep(1);
  system("diff -y pids_all pids_ps1 > pids");
  /*std::istringstream str(hpids);
  int pid;
  FILE * pid_fp = fopen("run/pids", "w");
  while(!str.eof()){
    str >> pid;
    fprintf(pid_fp, "%d\n", pid);
  }
  //std::cout << hpids << "Hello";
  //ofstream myfile;
  //myfile.open ("example.txt");
  //myfile << hpids;
  fclose(pid_fp);*/
}

void daemonise()
{
  pid_t pid, sid;
  FILE *pid_fp;

  syslog(LOG_INFO, "Starting daemonisation.");

  //First fork
  pid = fork();
  if(pid < 0)
  {
    syslog(LOG_ERR, "Error occured in the first fork while daemonising. Error number is %d", errno);
    exit(1);
  }

  if(pid > 0)
  {
    syslog(LOG_INFO, "First fork successful (Parent)");
    exit(0);
  }
  syslog(LOG_INFO, "First fork successful (Child)");

  //Create a new session
  sid = setsid();
  if(sid < 0) 
  {
    syslog(LOG_ERR, "Error occured in making a new session while daemonising. Error number is %d", errno);
    exit(1);
  }
  syslog(LOG_INFO, "New session was created successfuly!");

  //Second fork
  pid = fork();
  if(pid < 0)
  {
    syslog(LOG_ERR, "Error occured in the second fork while daemonising. Error number is %d", errno);
    exit(1);
  }

  if(pid > 0)
  {
    syslog(LOG_INFO, "Second fork successful (Parent)");
    exit(0);
  }
  pid = getpid();
  syslog(LOG_INFO, "Second fork successful (Child) %d", pid);

  

  //Change working directory to Home directory
  if(chdir(getenv("HOME")) == -1)
  {
    syslog(LOG_ERR, "Failed to change working directory while daemonising. Error number is %d", errno);
    exit(1);
  }

  //Grant all permisions for all files and directories created by the daemon
  umask(0);

  //Redirect std IO
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  if(open("/dev/null",O_RDONLY) == -1)
  {
    syslog(LOG_ERR, "Failed to reopen stdin while daemonising. Error number is %d", errno);
    exit(1);
  }
  if(open("/dev/null",O_WRONLY) == -1)
  {
    syslog(LOG_ERR, "Failed to reopen stdout while daemonising. Error number is %d", errno);
    exit(1);
  }
  if(open("/dev/null",O_RDWR) == -1)
  {
    syslog(LOG_ERR, "Failed to reopen stderr while daemonising. Error number is %d", errno);
    exit(1);
  }

  //Create a pid file
  mkdir("run/", 0777);
  pid_fp = fopen("run/daemon.pid", "w");
  if(pid_fp == NULL)
  {
    syslog(LOG_ERR, "Failed to create a pid file while daemonising. Error number is %d", errno);
    exit(1);
  }
  if(fprintf(pid_fp, "%d\n", pid) < 0)
  {
    syslog(LOG_ERR, "Failed to write pid to pid file while daemonising. Error number is %d, trying to remove file", errno);
    fclose(pid_fp);
    if(remove("run/daemon.pid") != 0)
    {
      syslog(LOG_ERR, "Failed to remove pid file. Error number is %d", errno);
    }
    exit(1);
  }
  while(1)
  {
    run_tool();
    sleep(10);
  }
  fclose(pid_fp);
}

int main()
{

  daemonise();
  //run_tool();
  //handle_signals();

//  while(1)
//  {
//    sleep(1000);
//  }
  return 0;
} 
